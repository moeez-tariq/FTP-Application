#include<stdio.h> // header contaning functions such as printf
#include<string.h> // header containing functions such as strcmp
#include<sys/socket.h> // header for socket functions
#include<arpa/inet.h> // header contaning byte-odering standardization functions of htons
#include<netinet/in.h> // header containing structure for internet addresses and macros such as INADDR_ANY
#include<dirent.h> // header containing functions to work with directories
#include<unistd.h> // header containing fork() and unix standard functions, also for unix specific functions declarations : fork(), getpid(), getppid()
#include<stdlib.h> // header containing functions such as malloc, free, atoi, exit

#include "../helper.h"

// defining 
#define PORT 6001
#define SIZE 256
#define DIR_SIZE 100
#define PACKET_SIZE 1024
#define DIRECTORY_LIST 2048

// attempted function to replace chains of send and receive, however gives errors with buffer so not used

// void send_command_receive_response(int serverfd, char *buffer)
// {
// 	int sendbytes = send(serverfd, buffer, strlen(buffer), 0); // send (STOR, RETR, LIST) request to server
// 	if(sendbytes < 0) // if error, report and exit
// 	{
// 		perror("send error");
// 		exit(-1);
// 	}

// 	// receive acknowledgement from server (150 File status OK)
// 	bzero(buffer,sizeof(buffer));
// 	int rcvbytes = recv(serverfd, buffer, sizeof(buffer), 0);
// 	if(rcvbytes < 0) // if error, report and exit
// 	{
// 		perror("receive error");
// 		exit(-1);
// 	}
// }

int main()
{

	int client_data_sd = 0; // create socket for data exchange (to be used later)
  	int server_data_sd = 0; // socket to connect with server (to be used later)

	//socket
	int serverfd = socket(AF_INET, SOCK_STREAM, 0);
	if(serverfd<0)
	{
		perror("socket error");
		exit(-1);
	}

	//setsock
	int value  = 1;
	if(setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) < 0) { //allows to reuse adress, without it one might run into "Address already in use" errors
		perror("setsockopt error");
        exit(-1);
	}

	struct sockaddr_in srvaddr; // defines server address
	bzero(&srvaddr,sizeof(srvaddr));
	srvaddr.sin_family = AF_INET; // IPV4 family
	srvaddr.sin_port = htons(PORT); // convert port number to network-byte order
	srvaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); //INADDR_ANY, INADDR_LOOP

	// connect to the server and if it fails give error
    if(connect(serverfd,(struct sockaddr*)&srvaddr,sizeof(srvaddr))<0)
    {
        perror("connect error");
        exit(-1);
    }
    
	// print the menu commands for FTP
	printf("Welcome! Please authenticate to be able to run server commands.\n");
	printf("1. type 'USER' followed by a space and your username.\n");
	printf("2. type 'PASS' followed by a space and your password to log in.\n");
	printf("type 'QUIT' to terminate your connection at any point.\n");
	printf("type '!LIST' to list all the files under the client directory.\n");
	printf("type '!CWD' followed by a space and directory name to change the current client directory \n");
	printf("type '!PWD' to display the the current client directory \n");
	printf("You can run the following commands once authenticated:\n");
	printf("'STOR' followed by a space and filename to send a file to the server \n");
	printf("'RETR' followed by a space and filename to download a file from the server \n");
	printf("'LIST' to list all the files under the current server directory\n");
	printf("'CWD' followed by a space and directory name to change the current server directory \n");
	printf("'PWD' to display the the current server directory\n\n");
	printf("220 Service ready for new user.\n");

	// declare the buffer to store user input
	char buffer[SIZE];
	bzero(buffer, sizeof(buffer));

	while (1) {
		printf("ftp> ");
		fgets(buffer, sizeof(buffer), stdin); // store user input in buffer
		buffer[strcspn(buffer, "\n")] = 0;  // remove trailing newline char from buffer, fgets does not remove it

		// copy buffer into tmp buffer, this is used later in case of data connections (STOR, RETR, LIST)
		char tmpbuffer[SIZE];
		bzero(tmpbuffer,sizeof(tmpbuffer));
		strcpy(tmpbuffer, buffer);

		// deal with !PWD command (give client directory)
		if(strncmp(buffer, "!PWD", 4)==0)
		{
			char current_directory[DIR_SIZE];
			printf("Current client directory: %s\n", getcwd(current_directory, SIZE));
		}

		// deal with !CWD command (change client directory)
		else if (strncmp(buffer, "!CWD", 4)==0) {
			char cwd_command[SIZE]; // stores the cwd command
			char directory[DIR_SIZE]; // stores the directory name
			// get directory from input, 
			sscanf(buffer, "%s %s", cwd_command, directory);

			// call change_directory function
			change_directory(directory);
		}

		else if (strcmp(buffer, "!LIST") == 0) {
			// get directory name
			char directory_name[DIR_SIZE];
			getcwd(directory_name, sizeof(directory_name));

			print_directory_contents(directory_name);
		}

		else if (strcmp(buffer, "QUIT") == 0)
		{
			// send QUIT command to server
			send(serverfd, buffer, strlen(buffer), 0);
			bzero(buffer,sizeof(buffer));

			// receive acknowledgement from server
			int bytes = recv(serverfd, buffer, sizeof(buffer),0);

			// if no acknowledgement received, error: server shutdown
			if (bytes <= 0){
				printf("error: server shutdown\n");
				return 0;
			}
			printf("%s\n", buffer);
			
			// close the relevant connections
			close(serverfd);
			close(client_data_sd);
			close(server_data_sd);
			bzero(buffer,sizeof(buffer));
			break;
		}


		// if user inputs LIST, STOR or RETR
		else if(strncmp(buffer, "LIST", 4) == 0 || strncmp(buffer, "STOR ", 5)==0 || strncmp(buffer, "RETR ", 5)==0)
		{

			// check for incorrect number of arguments with STOR and RETR
			if(strncmp(buffer, "STOR ", 5)==0 || strncmp(buffer, "RETR ", 5)) {
				int index = 0;
				int num_spaces = 0;

				for(index = 0; buffer[index] != '\0'; index++)
				{
					if (buffer[index] == ' ')
					{
						num_spaces++;
					}
				}

				// if there are more number of spaces than 1, then it is an invalid sequence
				if(num_spaces > 1){
					printf("503 Bad sequence of commands.\n");
					continue;
				}
			}

			// if user inputs STOR, check if file exists
			if(strncmp(buffer, "STOR ", 5)==0){

				char stor[10]; // store the STOR command
				char filename[100]; // store the filename
				sscanf(buffer, "%s %s", stor, filename);

				FILE *fp = fopen(filename, "r");
				// if the file can not be opened
				if (fp == NULL) {
					printf("550 No such file or directory.\n");
					close(server_data_sd);
					// fclose(fp);
					continue;
				}
			}

			// if everything good, send a PORT command to ask for a port
			send(serverfd, "PORT", 4, 0);
			
			// channel on data port to connect to (N+i)
			int channel;
			int rec_bytes = recv(serverfd, &channel, sizeof(channel), 0);

			// check if server shutdown
			if (rec_bytes <= 0){
				printf("error: server shutdown\n");
				return 0;
			}

			// specify address of where to make data connection
			struct sockaddr_in curr_addr;
			socklen_t addr_len = sizeof(curr_addr);

			// retrieve the port number over which the socket for control connection runs
			if (getsockname(serverfd, (struct sockaddr*) &curr_addr, &addr_len) != 0) {
				perror("Cannot read socket information.\n");
				continue;
			}

			// set the port and ip for connection
			char port_request[SIZE]; // string to store the port request
			bzero(port_request, sizeof(port_request));

			int h1, h2, h3, h4, p1, p2;

			// increment the port number by channel to ensure a new port is used for data connection
			int client_port = ntohs(curr_addr.sin_port) + channel;

			// get the ip address of control connection
			char* client_ip = inet_ntoa(curr_addr.sin_addr);

			// divide the port into p1 and p2 to be sent to the server
			p1 = client_port / 256;
			p2 = client_port % 256;

			// divide the client_ip into h1, h2, h3, h4 to be sent to the server
			sscanf(client_ip, "%u.%u.%u.%u", &h1,&h2,&h3,&h4);

			// populate variable h1 to h4 and p1 to p2 and send the port request to the server
			snprintf(port_request, SIZE, "PORT %u %u %u %u %u %u", h1, h2, h3, h4, p1, p2);

			int bytes = send(serverfd, port_request, sizeof(port_request), 0);

			if(bytes < 0) // if error in sending request, exit
			{
				perror("send failed");
				exit(-1);
			}

			// create socket for data exchange
			client_data_sd = socket(AF_INET,SOCK_STREAM,0);

			if (client_data_sd < 0){
				perror("data sock: ");
				exit(-1);
			}

			struct sockaddr_in data_addr;
			memset(&data_addr, 0, sizeof(data_addr));

			data_addr.sin_family = AF_INET;
			data_addr.sin_port = htons(client_port);
			data_addr.sin_addr.s_addr = inet_addr(client_ip);

			int value  = 1;
			setsockopt(client_data_sd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));

			// bind the data port
			if (bind(client_data_sd, (struct sockaddr *) &data_addr, sizeof(data_addr)) < 0)
			{
				perror("bind failed"); // throw error if bind fails
				exit(-1);
			}

			// listen on the data socket
			if (listen(client_data_sd, 5) < 0)
			{
				perror("listen failed"); // if listen fails, throw error and close the data socket
				close(client_data_sd);
				exit(-1);
			}

			char port_response[SIZE];
			recv(serverfd, port_response, sizeof(port_response), 0); // receive acknowledgement from server
			printf("%s\n", port_response);

			// if acknowledgement received, accept the data connection
			if (strncmp(port_response, "200", 3) == 0){ 
				
				server_data_sd = accept(client_data_sd, (struct sockaddr *)&data_addr, (socklen_t*)&addr_len);

				if (server_data_sd < 1)
				{
					perror("failure to accept data connection"); // if error, report it
					exit(-1);
				}

			}

			// deal with STOR request
			if(strncmp(buffer, "STOR", 4)==0) {
				int sendbytes = send(serverfd, buffer, strlen(buffer), 0); // send STOR request to server
				if(sendbytes < 0) // if error, report and exit
				{
					perror("send error");
					exit(-1);
				}

				// receive acknowledgement from server (150 File status OK)
				bzero(buffer,sizeof(buffer));
				int rcvbytes = recv(serverfd, buffer, sizeof(buffer), 0);
				if(rcvbytes < 0) // if error, report and exit
				{
					perror("receive error");
					exit(-1);
				}

				// send_command_receive_response(serverfd, buffer);

				// if acknowledgement received (150 file status ok)
				if(strncmp(buffer, "150", 3)==0)
				{
					printf("%s", buffer); // print acknowledgement

					// here we use the tmpbuffer we declared at the beginning
					// it contains the command and filename
					char cmd[10]; // store the STOR command
					char filename[100]; // store the filename
					sscanf(tmpbuffer, "%s %s", cmd, filename);

					// read the file
					FILE *fp = fopen(filename, "r");
					if (fp == NULL) { // if file pointer null, give error and exit
						perror("Can not read file");
						exit(-1);
					}

					// seek to end of file
					fseek(fp, 0, SEEK_END);
					// get file size
					int file_size = ftell(fp);
					// reset file ptr
					fseek(fp, 0, SEEK_SET);

					bzero(tmpbuffer,sizeof(tmpbuffer));

					// the data in the file
					char *data = malloc(file_size);

					// read entire file
					fread(data, 1, file_size, fp);

					// bytes sent
					unsigned int sent = 0;

					// bytes that need to be sent
					unsigned int to_send = 0;

					// send 1024 bytes each time until all are sent
					while(sent < file_size)
					{
						to_send = file_size - sent;
						if(to_send >= PACKET_SIZE)
						{
							to_send = PACKET_SIZE;
						}
						send(server_data_sd, data + sent, to_send, 0);
						sent += to_send; // update the number of bytes sent
					}

					// close data socket after file transmission
					close(server_data_sd);
					// close file after file transmission
					fclose(fp);

					// receive acknowledgement from server after file has been STORed
					bzero(tmpbuffer,sizeof(tmpbuffer));
					recv(serverfd, tmpbuffer, sizeof(tmpbuffer), 0);
					printf("%s\n",tmpbuffer); // printing "transfer complete" etc

				}

			}

			// handle RETR command
			if(strncmp(buffer, "RETR", 4) == 0) {
			
				int sendbytes = send(serverfd, buffer, strlen(buffer), 0); // send RETR request to server
				// if error, report and exit
				if(sendbytes < 0)
				{
					perror("send");
					exit(-1);
				}

				// receive acknowledgement from server (150 File status OK)
				bzero(buffer,sizeof(buffer));
				int rcvbytes = recv(serverfd, buffer, sizeof(buffer), 0);
				// if error, report and exit
				if(rcvbytes<0)
				{
					perror("receive");
					exit(-1);
				}

				// send_command_receive_response(serverfd, buffer);

				if(strncmp(buffer, "150", 3)==0) { // if acknowledegement received

					printf("%s", buffer); // print the acknowledgement

					// here we use the tmpbuffer we declared at the beginning
					// it contains the command and filename
					char cmd[10]; // store the command (RETR)
					char filename[100]; // store the filename
					sscanf(tmpbuffer, "%s %s", cmd, filename);

					int bytes_received = 0;
					// first open the file in write mode
					// this creates a new file of filename if it doesnt exit and overwrites old content if it exists
					FILE *fp = fopen(filename, "w");
					if (fp == NULL) {
						perror("Can not write to file");
						exit(-1);
					}
					fclose(fp); // close this file pointer
 
					// now open the file in append mode
					fp = fopen(filename, "a+");

					char buf[PACKET_SIZE]; // create buffer to receive bytes

					// receive all bytes
					while (bytes_received >= 0)
					{
						bytes_received = recv(server_data_sd, buf, PACKET_SIZE, 0);
						// write to new file
						fwrite(buf, bytes_received, 1, fp);
						if(bytes_received < PACKET_SIZE){ // if last loop of transmission, get out of while loop
							break;
						}
					}

					// close data connection upon transmission
					close(server_data_sd);
					//close file upon file transmission
					fclose(fp);

					// receive acknowledgement from server after file has been RETRed
					bzero(tmpbuffer,sizeof(tmpbuffer));
					recv(serverfd, tmpbuffer, sizeof(tmpbuffer), 0);
					printf("%s\n",tmpbuffer); // printing "transfer complete" etc

				}

				if(strncmp(buffer, "550", 3)==0) {
					printf("%s\n", buffer);
					close(server_data_sd);
				}

			}

			if (strncmp(buffer,"LIST",4)==0) {

				int sendbytes = send(serverfd, buffer, strlen(buffer), 0); // send LIST request to server
				// if error, report and exit
				if(sendbytes < 0)
				{
					perror("send");
					exit(-1);
				}

				// receive acknowledgement from server (150 File status OK)
				bzero(buffer,sizeof(buffer));
				int rcvbytes = recv(serverfd, buffer, sizeof(buffer), 0);
				// if error, report and exit
				if(rcvbytes<0)
				{
					perror("receive");
					exit(-1);
				}

				// send_command_receive_response(serverfd, buffer);

				// if acknowledgement received
				if(strncmp(buffer, "150", 3)==0) {

					printf("%s", buffer); // print the acknowledgement

					int list_size;
					char list_contents[DIRECTORY_LIST];
					
					bzero(list_contents,sizeof(list_contents));

					recv(server_data_sd,&list_size,sizeof(int),0); // receive directory file info
					recv(server_data_sd,list_contents,list_size,0); // receive the actual list
					printf("%s\n",list_contents);
					// close data socket upon file transmission
					close(server_data_sd);

					// receive acknowledgement from server that operation is completed
					bzero(tmpbuffer,sizeof(tmpbuffer));
					recv(serverfd, tmpbuffer, sizeof(tmpbuffer),0);
					printf("%s\n", tmpbuffer); // printing "transfer complete" etc

				}

			}

		}

		// dealing with PWD, CWD, USER and PASS commands
		else if (strncmp(buffer, "PWD", 3)==0 || strncmp(buffer, "CWD", 3)==0 || strncmp(buffer, "USER", 4)==0 || strncmp(buffer, "PASS", 4)==0) {
			// send command on control socket
			send(serverfd,buffer,sizeof(buffer),0);

			// receive response from the server and print it
			// the specific response is handled at the server side
			bzero(buffer,sizeof(buffer));
			int rec_bytes = recv(serverfd,buffer,sizeof(buffer),0);
			if (rec_bytes<=0){
				printf("error: server shutdown\n");
				return 0;
			}
			printf("%s\n",buffer);
		}

		else { // in case the command is something not implemented
			// send command on control socket
			send(serverfd,buffer,sizeof(buffer),0);

			// receive response from the server and print it
			bzero(buffer,sizeof(buffer));
			int rec_bytes = recv(serverfd,buffer,sizeof(buffer),0);
			if (rec_bytes<=0){
				printf("error: server shutdown\n");
				return 0;
			}
			printf("%s\n",buffer); // command not implemented
		}

	}

	return 0;
}
