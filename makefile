all:
	gcc client_directory/client.c -o client_directory/client
	gcc server_directory/server.c -o server_directory/server

client: client_directory/client.c
	gcc client_directory/client.c -o client_directory/client

server: server_directory/server.c
	gcc server_directory/server.c -o server_directory/server

clean:
	rm -f client_directory/client server_directory/server