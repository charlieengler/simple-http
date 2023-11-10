#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

int errno;

#define RCVBUFSIZE 1024
#define HTTP_VERSION "1.1"
#define INDEX_FILE "./TMDG.html"

int send_http_error(int, int);

#define NUM_ALLOWED_METHODS 1
const char *allowed_methods[NUM_ALLOWED_METHODS] = { "GET" };
#define NUM_DISALLOWED_METHODS 4
const char *disallowed_methods[NUM_DISALLOWED_METHODS] = { "POST", "HEAD", "PUT", "DELETE" };

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        fprintf(stderr, "usage http_server port_number\n");
        return 1;
    }

    char *input_port = argv[1];

    int status;
    struct addrinfo hints;
    struct addrinfo *serv_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((status = getaddrinfo(NULL, input_port, &hints, &serv_info)) != 0)
    {
        fprintf(stderr, "getaddrinfo() error: %s\n", gai_strerror(status));
        exit(1);
    }

    int sockfd;
    if((sockfd = socket(serv_info->ai_family, serv_info->ai_socktype, serv_info->ai_protocol)) == -1)
    {
        printf("socket() error: %s\n", strerror(errno));
        exit(1);
    }

    int yes = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        printf("setsockopt() error\n");
        exit(1);
    }

    int bind_status;
    if((bind_status = bind(sockfd, serv_info->ai_addr, serv_info->ai_addrlen)) == -1)
    {
        printf("bind() error: %s\n", strerror(errno));
        exit(1);
    }

    int listen_status;
    if((listen_status = listen(sockfd, 5)) == -1)
    {
        printf("listen() error: %s\n", strerror(errno));
        exit(1);
    }

    struct sockaddr_storage incoming_addr;
    for(;;)
    {
        socklen_t addr_size = sizeof(incoming_addr);
        int new_fd;
        close(new_fd);
        if((new_fd = accept(sockfd, (struct sockaddr *)&incoming_addr, &addr_size)) == -1)
        {
            printf("accept() error: %s\n", strerror(errno));
            continue;
        }

        char buf[RCVBUFSIZE];

        int read_size;
        if((read_size = recv(new_fd, buf, sizeof(buf), 0)) <= 0)
        {
            send_http_error(400, new_fd);

            if(read_size == 0)
            {
                printf("recv() error: no header received\n");
            }
            else
            {
                printf("recv() error: %s\n", strerror(errno));
            }

            continue;
        }

        buf[RCVBUFSIZE] = 0;

        int found_allowed_method = 0;
        int found_disallowed_method = 0;

        for(int i = 0; i < NUM_ALLOWED_METHODS; i++)
        {
            if(strstr(buf, allowed_methods[i]) != NULL)
            {
                found_allowed_method = 1;
            }
        }

        for(int i = 0; i < NUM_DISALLOWED_METHODS; i++)
        {
            if(strstr(buf, disallowed_methods[i]) != NULL)
            {
                found_disallowed_method = 1;
            }
        }

        if(found_disallowed_method == 1)
        {
            send_http_error(405, new_fd);

            printf("Unknown HTTP Method in incoming header\n");
            continue;
        }

        if(found_allowed_method == 0 && found_disallowed_method == 0)
        {
            send_http_error(400, new_fd);

            printf("Bad HTTP Request in incoming header\n");
            continue;
        }

        char *raw_path = strchr(buf, '/');
        char trimmed_path[RCVBUFSIZE+1];
        trimmed_path[0] = '.';
        for(int i = 0; i < strlen(raw_path); i++)
        {
            if(raw_path[i] == ' ')
            {
                trimmed_path[i+1] = 0;

                break;
            }

            trimmed_path[i+1] = raw_path[i];
        }

        char *http_version;
        if((http_version = strstr(buf, "HTTP/")) != NULL)
        {
            if(strstr(http_version, HTTP_VERSION) == NULL)
            {
                send_http_error(505, new_fd);

                printf("Unknown HTTP Version in incoming header\n");
                continue;
            }
        }
        else
        {
            send_http_error(400, new_fd);

            printf("Bad HTTP Request in incoming header\n");
            continue;
        }

        char *find_path = trimmed_path;
        if(strcmp(trimmed_path, "./") == 0 || strcmp(trimmed_path, "./index.html") == 0)
        {
            find_path = INDEX_FILE;
        }

        if(access(find_path, F_OK) != 0)
        {
            send_http_error(404, new_fd);

            printf("File doesn't exist on server\n");
            continue;
        }

        if(strcmp(find_path, INDEX_FILE) != 0)
        {
            send_http_error(401, new_fd);

            printf("Client tried to access unauthorized file\n");
            continue;
        }

        char *send_buffer = 0;
        long send_length;
        FILE *send_file = fopen(find_path, "rb");

        if(send_file)
        {
            fseek(send_file, 0, SEEK_END);
            send_length = ftell(send_file);
            fseek(send_file, 0, SEEK_SET);
            send_buffer = malloc(send_length + 1);
            if(send_buffer)
            {
                fread(send_buffer, 1, send_length, send_file);
            }

            fclose(send_file);

            send_buffer[send_length] = 0;
        }

        char *response_header = "HTTP/1.1 200 OK\r\n\r\n";
        char *response_buffer = malloc((strlen(response_header) + strlen(send_buffer)) * sizeof(char));
        strcpy(response_buffer, response_header);
        strcat(response_buffer, send_buffer);

        long total_send_size = 0;
        while(total_send_size < strlen(response_buffer))
        {
            int send_size = send(new_fd, &response_buffer[total_send_size], strlen(response_buffer)-total_send_size, 0);
            if(send_size == -1)
            {
                send_http_error(500, new_fd);

                printf("send() error\n");
            }

            total_send_size += send_size;
        }

        char ip[NI_MAXHOST];
        char port[NI_MAXSERV];
        getnameinfo((struct sockaddr*)&incoming_addr, sizeof(struct sockaddr_storage), ip, sizeof(ip), port, sizeof(port), 0);

        printf("Sent %ld bytes to %s:%s\n", total_send_size, ip, port);

        free(send_buffer);
        free(response_buffer);
    }

    close(sockfd);

    return 0;
}

int send_http_error(int error, int sock)
{
    char *header = "HTTP/1.1 418 I'm a teapot\r\n\r\n";

    switch(error)
    {
        case 400:
            header = "HTTP/1.1 400 Bad Request\r\n\r\n";
            break;
        case 401:
            header = "HTTP/1.1 401 Unauthorized\r\n\r\n";
            break;
        case 404:
            header = "HTTP/1.1 404 Not Found\r\n\r\n";
            break;
        case 405:
            header = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
            break;
        case 500:
            header = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
            break;
        case 505:
            header = "HTTP/1.1 505 HTTP Version Not Supported\r\n\r\n";
            break;
    }

    int send_size = send(sock, header, strlen(header), 0);
    if(send_size == -1)
    {
        printf("send() error during send_http_error()\n");
        return 1;
    }
    else if(send_size != strlen(header))
    {
        printf("send() error during send_http_error(): size of header doesn't match size of data sent\n");
        return 1;
    }

    close(sock);

    return 0;
}