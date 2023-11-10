#include <stdio.h>
#include <stdlib.h>    
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

int errno;

#define RCVBUFSIZE 100000

int ping(int, struct addrinfo*);
int save_to_file(int, char*);
void create_table(long double*, long double);

int main(int argc, char *argv[])
{
    if(argc < 3 || argc > 4)
    {
        fprintf(stderr, "usage: http_client [-options] server_url port_number\n");
        return 1;
    }

    char *input_address = argv[1];
    char *input_port = argv[2];

    int PING_MODE = 0;
    if(strcmp(argv[1], "-p") == 0)
    {
        input_address = argv[2];
        input_port = argv[3];

        PING_MODE = 1;
    }

    int len_diff = strlen(input_address);
    char *input_path;
    if((input_path = strchr(input_address, '/')) == NULL)
    {
        input_path = "/";
    }
    else
    {
        len_diff = strlen(input_address) - strlen(input_path);
    }

    char *trimmed_address = malloc(len_diff * sizeof(char));

    for(int i = 0; i < len_diff; i++)
    {
        trimmed_address[i] = input_address[i];
    }

    struct addrinfo hints;
    struct addrinfo *serv_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int status;
    if((status = getaddrinfo(trimmed_address, input_port, &hints, &serv_info)) != 0)
    {
        fprintf(stderr, "getaddrinfo() error: %s\n", gai_strerror(status));
        exit(1);
    }

    int sockfd;
    if((sockfd = socket(serv_info->ai_family, serv_info->ai_socktype, serv_info->ai_protocol)) < 0)
    {
        printf("socket() error: %s\n", strerror(errno));
        exit(1);
    }

    int connection;
    if((connection = connect(sockfd, serv_info->ai_addr, serv_info->ai_addrlen)) == -1)
    {
        printf("connect() error: %s\n", strerror(errno));
        exit(1);
    }

    char *header_part_one = "GET "; 
    char *header_part_two = " HTTP/1.1\r\nHost: ";
    char *header_part_three = "\r\nConnection: close\r\n\r\n";

    char *header = malloc(sizeof(char));
    
    strcpy(header, header_part_one);
    strcat(header, input_path);
    strcat(header, header_part_two);
    strcat(header, trimmed_address);
    strcat(header, header_part_three);

    int save_status;
    if((save_status = save_to_file(sockfd, header)) != 0)
    {
        printf("save_to_file() error\n");
        exit(1);
    }

    close(sockfd);

    if(PING_MODE)
    {
        int ping_status;
        if((ping_status = ping(sockfd, serv_info)) != 0)
        {
            printf("ping() error\n");
            exit(1);
        }
    }

    free(header);

    return 0;
}

int ping(int sockfd, struct addrinfo *serv_info)
{
    struct timespec time_start, time_end;

    long double rtts[10];
    long double average_rtt = 0;
    for(int i = 0; i < 10; i++)
    {
        int sockfd;
        if((sockfd = socket(serv_info->ai_family, serv_info->ai_socktype, serv_info->ai_protocol)) < -1)
        {
            printf("socket() error: %s\n", strerror(errno));
            exit(1);
        }
        
        clock_gettime(CLOCK_MONOTONIC, &time_start);

        int connection;
        if((connection = connect(sockfd, serv_info->ai_addr, serv_info->ai_addrlen)) == -1)
        {
            printf("connect() error during ping(): %s\n", strerror(errno));
            exit(1);
        }

        close(sockfd);

        clock_gettime(CLOCK_MONOTONIC, &time_end);

        double elapsed = ((double)(time_end.tv_nsec - time_start.tv_nsec))/1000000.0;
        long double rtt = (time_end.tv_sec-time_start.tv_sec) * 1000.0 + elapsed;

        rtts[i] = rtt;
        average_rtt += rtt;
    }

    average_rtt /= 10.0;

    create_table(rtts, average_rtt);

    return 0;
}

int save_to_file(int sockfd, char *full_header)
{
    int send_size = send(sockfd, full_header, strlen(full_header), 0);
    if(send_size == -1)
    {
        printf("send() error during save_to_file()\n");
        return 1;
    }
    else if(send_size != strlen(full_header))
    {
        printf("send() error during save_to_file(): size of header doesn't match size of data sent\n");
        return 1;
    }

    char buf[RCVBUFSIZE];

    int recv_size = 0;
    int total_size = 0;
    while((recv_size = recv(sockfd, &buf[total_size], sizeof(buf)-total_size, 0)) > 0)
    {
        total_size += recv_size;
    }
    
    buf[total_size] = 0;

    printf("Received %d bytes of data\n", total_size);

    printf("Response:\n%s\n", buf);

    FILE *f;
    f = fopen("index.html", "w");
    fprintf(f, buf);
    fclose(f);

    printf("Saved to page contents to index.html\n");

    return 0;
}

void create_table(long double *rtts, long double average_rtt)
{
    printf("___________________\nRTT Table:\n___________________\n");

    for(int i = 0; i < 10; i++)
    {
        printf("RTT %d: %2.2Lfms\n", i+1, rtts[i]);
    }

    printf("Average RTT: %2.2Lfms\n", average_rtt);
    printf("___________________\n");
}