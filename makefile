all: client server

client: ./client/client.o
	gcc ./client/client.o -o http_client

client.o: ./client/client.c
	gcc -c ./client/client.c -o ./client/client.o

server: ./server/server.o
	gcc ./server/server.o -o http_server

server.o: ./server/server.c
	gcc -c ./server/server.c -o ./server/server.o

clean:
	rm http_client ./client/client.o http_server ./server/server.o