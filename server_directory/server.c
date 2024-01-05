#include<stdio.h> //header containing functions such as printf
#include<string.h> //header containing functions such as memset
#include<sys/socket.h> //header containing socket functions
#include<arpa/inet.h> //header containing byte-ordering standardization functions of htons
#include <netinet/in.h> //header containing structure for internet addresses and macros such as INADDR_ANY
#include <sys/stat.h>//header for mkdir
#include <sys/types.h>//header for mkdir
#include<unistd.h> //for POSIX operating system API functions such as close
#include<stdlib.h> //header containing functions such as malloc, free, atoi
#include <dirent.h> //header for directory functions
#include <stdbool.h> //header for boolean library

//defines constant sizes reused again and again
#define PASSWORD_LENGTH 100
#define DIRECTORY_LENGTH 500
#define SIZE 256

//define global variable
int channels = 0;
int totalClients = 0;

//data structure that contains elements of clients
typedef struct
{
	int fd;
	char* username;
	bool authenticated;
	char workingDirectory[DIRECTORY_LENGTH];
} client;

// Array of structure to store information of 5 clients
client client_array[5];

//will get contain the server working directory
char serverCWD[DIRECTORY_LENGTH];

//sends normal messages to client (not data)
void sendMessageToClient(int client_fd, char* message)
{
	int bytes = send(client_fd, message, strlen(message), 0);
	if (bytes < 0)
	{
		perror("SENDING FAILED\n");
		exit(-1);
	}
}

//authenticates user login information for username and password
void userLoginCheck(int client_fd, client client_array[5], char* username, int index)
{
	//opens the file users.csv to read the username and password already stored
	FILE* fptr;
	fptr = fopen("../users.csv", "r");

	//if the file does not open, then give an error
	if (!fptr)
	{
		perror("USER FILE NOT OPENED\n");
		exit(-1);
	}

	//defines variables
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	char* fileUsername;
	char* filePassword;
	char* password;
	bool fileUserFound = false;
	char message[SIZE];

	while (1)
	{
		fileUserFound = false;
		read = getline(&line, &len, fptr); //keep reading lines one by one
		if (read == -1) //if end of file then break
		{
			break;
		}

		char* data = strtok(line, ","); //breaks the line when "," is encountered
		fileUsername = data; //add the username to the correct variable
		while (data != NULL)
		{
			data = strtok(NULL, "\n");
			filePassword = data; //add the password to the right variable
			break;
		}

		if (strcmp(fileUsername, username) == 0) //if the fileusername and username match
		{
			fileUserFound = true;
			bzero(&message, sizeof(message));
			strcpy(message, "331 Username OK, Need Password\n"); //then send this message
			printf("%s\n",message);
			sendMessageToClient(client_fd, message); 

			if (fileUserFound == true)
			{
				break;
			}
		}
	}

	if (fileUserFound == true) //if username is found in the list
	{
		bzero(&message, sizeof(message));
		int bytes = recv(client_fd, message, sizeof(message), 0); //recieve the PASS command and password from the client
		if (bytes < 0)
		{
			perror("FAILED TO RECEIVE\n");
			exit(-1);
		}

		if (strncmp(message, "PASS", 4) == 0)
		{
			char* data = strtok(message, " "); //when space is encountered then break the line in two
			char* temp;
			temp = data; //PASS command is stored in temp
			while (data != NULL)
			{
				data = strtok(NULL, "\n");
				password = data; //the password is stored in password
				break;
			}

			if (strcmp(filePassword, password) == 0) //compare the file password and input password, if they match
			{
				printf("230 User Logged In, Proceed\n");
				sendMessageToClient(client_fd, "230 User Logged In, Proceed\n"); //send the message to client that user is logged in

				client_array[index].username = username; //set the username of the client in the client array
				client_array[index].authenticated = true; //since the user is logged in, the client has been authenticated

				//if the directory has not been made, then make it
				if (!mkdir(client_array[index].username,0777))
				{
					printf("Directory Created\n");
				}
				else
				{
					printf("Directory Already Exists\n"); //else if the directory already exists, print this
				}

				//enter the directory and give an error if it doesnt happen
				if (chdir(client_array[index].username) == -1) 
				{
					printf("Could Not Enter Client Directory\n");
					close(client_fd);
				}

				//get the current working directory and store in the client's working directory
				getcwd(client_array[index].workingDirectory, DIRECTORY_LENGTH);

				if (chdir("..") == -1) 
				{
					printf("Could Not Enter User Directory\n");
					close(client_fd);
				}
			}
			else
			{
				printf("530 Passwords Dont Match, Try Again\n");
				sendMessageToClient(client_fd, "530 Passwords Dont Match, Try Again\n"); //give an error if the username and password doesnt match
			}

		}
		else
		{
			printf("530 Not Logged In\n");
			sendMessageToClient(client_fd,"530 Not Logged In\n"); //if a command other than pass is given after the USER command
		}
	}
	else
	{
		bzero(&message, sizeof(message));
		strcpy(message, "530 Username Not Found\n"); //if username is not found in the file
		sendMessageToClient(client_fd, message);
	}

	fclose(fptr); //close file
	if (line)
	{
		free(line);	
	}
}

//changes the server directory
void changeDirectory(int client_fd, char* directory, client client_array[5], int index)
{
	strcat(client_array[index].workingDirectory, "/"); //adds a / to the working directory
	strcat(client_array[index].workingDirectory, directory); //then adds the directory name
	char message[SIZE];
	strcpy(message, "200 Directory Changed To "); 
	//message = "200 Directory Changed To ";
	strcat(message, client_array[index].workingDirectory);
	sendMessageToClient(client_fd, message); //sends confirmation to client that directory has been changed
}

//displays current server directory
void displayDirectory(int client_fd, client client_array[5], int index)
{
	char message[DIRECTORY_LENGTH];
	strcpy(message, "257 ");
	//message = "257 ";
	strcat(message, client_array[index].workingDirectory);
	strcat(message, "\n");
	sendMessageToClient(client_fd, message); //sends confirmation to the client with the code 257
}

void displayList(int client_fd, client client_array[5], int index, int new_sd)
{
	DIR* currentDirectory;
	char message[DIRECTORY_LENGTH];
	bzero(&message, sizeof(message));
	currentDirectory = opendir(client_array[index].workingDirectory); //opens a current directory
	sendMessageToClient(client_fd, "150 File Status Okay; About To Open Data Connection.\n"); //sends acknowledgement message to client

	if (currentDirectory) //if currentDirectory is opened properly
	{
		struct dirent* direc;
		char* listElement; //declare a variable
		while (1)
		{
			direc = readdir(currentDirectory); //keep reading the directory
			if (direc == NULL)
			{
				break;
			}
			else
			{
				bzero(&listElement, sizeof(listElement));
				listElement = direc->d_name; //save the name in listElement
				if (strcmp(direc->d_name, "..") == 0) //if the name is ..
				{
					continue;
				}
				else if (strcmp(direc->d_name, ".") == 0) //or if the name is .
				{
					continue; //then ignore these, dont print and continue
				}
				else
				{
					strcat(listElement, "\n"); //add \n to the end so that it is displayed in a neat order
					strcat(message, listElement); //add the name to the message buffer
				}
			}
		}
		int size = sizeof(message);
		send(new_sd, &size, sizeof(size), 0); //send the size of message to the client
		send(new_sd, message, sizeof(message), 0); //then send the message itself

		closedir(currentDirectory); //close the directory
		sendMessageToClient(client_fd, "226 Transfer Completed\n"); //send acknowledgement that transfer is complet
		exit(0);
	}
}

//stores file from the client to the server
void storeFile(int client_fd, client client_array[5], int index, char* file, int new_sd)
{

	char fileDirectory[DIRECTORY_LENGTH];
	strcpy(fileDirectory, client_array[index].workingDirectory);
	strcat(fileDirectory, "/");
	strcat(fileDirectory, file); //ensure that fileDirectory contains the full path and name

	FILE* fptr = fopen(fileDirectory, "wb"); //open it in write binary mode
	if (!fptr)
	{
		perror("FILE OPENING FAILED\n"); //give error if it fails to open
		exit(-1);
	}

	sendMessageToClient(client_fd, "150 File Status Okay; About To Open Data Connection.\n"); //sends acknowdgement to the client

	char data[SIZE * 4];
	int data_received = 0;

	while (1) //keep receiving data
	{
		data_received = recv(new_sd, data, sizeof(data), 0); //receive a chunk of data
		fwrite(data,data_received, 1, fptr); //write it in the file
		if (data_received < (SIZE * 4)) //we are expecting a chunck of 1024 bytes at once so if it is less than that then that means the data on the client end is finished
		{
			fclose(fptr); //close the file
			sendMessageToClient(client_fd, "226 Transfer Completed\n"); //tell the client that data transfer is complete
			printf("Data Connection Closed\n");
			close(new_sd); //close the new_sd
			break; //break out of the loop and eventually out of the function
		}
	}
	exit(0);
}

//retrieves file from the server to the client
void retrieveFile(int client_fd, client client_array[5], int index, char* file, int new_sd) 
{
	char fileDirectory[DIRECTORY_LENGTH];
	strcpy(fileDirectory, client_array[index].workingDirectory);
	strcat(fileDirectory, "/");
	strcat(fileDirectory, file); //ensures proper path with file name

	FILE* fptr;
	fptr = fopen(fileDirectory, "rb"); //opens the file to read it in binary mode
	if (!fptr)
	{
		sendMessageToClient(client_fd,"550 No Such File or Directory\n"); //send message if the file does not exist or fails to open
		close(new_sd);
	}
	else
	{
		sendMessageToClient(client_fd, "150 File Status Okay; About To Open Data Connection.\n"); //else send acknowledgement
		struct stat stats;
		int size = 0;
		int bytes_sent = 0;
		char buffer[SIZE * 4];
		if (stat(fileDirectory, &stats) == 0) //gets the stats of the file
		{
			size = stats.st_size; //stores the size of the file
			while (bytes_sent < size) //keep looping till all the bytes are send
			{
				if (size - bytes_sent >= 1024) //if more than 1024 bytes of data exists
				{
					fread(buffer, 1024, 1, fptr); //then read 1024 bytes
					bytes_sent = bytes_sent + 1024; //increment counter accordingly
				}
				else
				{
					fread(buffer, (size - bytes_sent), 1, fptr); //otherwise read the remaining bytes
					bytes_sent = bytes_sent + (size - bytes_sent); //and increment the counter accordingly
				}

				send(new_sd, buffer, sizeof(buffer), 0); //send whatever has been read
			}

			fclose(fptr); //close the file
			sendMessageToClient(client_fd, "226 Transfer Completed\n"); //send acknowledgement
			printf("Data Connection Closed\n");
			close(new_sd); //close new_sd
			// close(new_sd);
			exit(0);
		}
	}
}

// void internalPortCommand(int client_fd, char* message, int new_sd)
// {
// 	char portInformation[SIZE];
// 	strcpy(portInformation, message);
// 	unsigned int h1,h2,h3,h4,p1,p2;
// 	sscanf(portInformation, "PORT %u %u %u %u %u %u", &h1,&h2,&h3,&h4,&p1,&p2);
// 	bzero(portInformation,sizeof(portInformation));
// 	sprintf(portInformation, "%u.%u.%u.%u",h1,h2,h3,h4);
// 	unsigned int newdata_port = (p1 * SIZE) + p2;

// 	struct sockaddr_in user_addr, target_addr;
// 	socklen_t addr_len = sizeof(user_addr);

// 	user_addr.sin_family = AF_INET;
// 	user_addr.sin_port = htons(6000);
// 	user_addr.sin_addr.s_addr = inet_addr(portInformation);

// 	// Initializing the port and IP address of the socket structure
// 	target_addr.sin_family = AF_INET;
// 	target_addr.sin_port = htons(newdata_port);
// 	target_addr.sin_addr.s_addr = inet_addr(portInformation);

// 		// Opening a new Socket for the Data Connection
// 	if( (new_sd = socket(AF_INET , SOCK_STREAM , 0)) == 0) { // Create new socket
// 			perror("SOCKET CREATION FAILED");
// 			exit(1);
// 	}
// 	else
// 	{
// 		printf("SOCKET CREATED\n");
// 	}

// 	int value  = 1;
// 	setsockopt(new_sd,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value));

// 	// Binding the Data Port
// 	if(bind(new_sd, (struct sockaddr*)&user_addr,sizeof(user_addr)) < 0)
// 	{
// 		// If the bind fails, an error is thrown to the user
// 		perror("BIND FAILED");
// 		exit(-1);
// 	}

// 	// Connecting with the client on the Data Connection
// 	if (connect(new_sd, (struct sockaddr *)&target_addr, sizeof(target_addr)) < 0) 
// 	{ // Connect to target
// 			perror("CONNECTION FAILED");
// 			close(new_sd);
// 			exit(-1);
// 	}
// 	printf("CONNECTION ESTABLISHED WITH CLIENT DATA CHANNEL \n");

// 	// Sending an Acknowledgmenet if everything works well!
// 	sendMessageToClient(client_fd, "200 PORT COMMAND SUCCESSFUL\n");
// }


//=================================
//client commands are looked at here
int clientCommands(int client_fd)
{
	char receivedMessage[256];
	bzero(&receivedMessage,sizeof(receivedMessage));

	int n; //index of client
	int new_sd;
	// if a cient is looking to establish a connection over current request, find
    for (int i = 0; i < 5; i++) //5 because we usually have a max of 5 clients
    {
        if (client_array[i].fd == client_fd)
        {
            n = i;
            break;
        }
    }

    //receive the command and the extension of the command
	int bytes = recv(client_fd, receivedMessage, sizeof(receivedMessage), 0);
    //if the message is not properly received, give an error
	if(bytes < 0)
	{ 
		perror("MESSAGE NOT RECEIVED"); 
		return 0;
	}

    //if the message received is QUIT or is an empty message
	if(strcmp(receivedMessage,"QUIT")==0 || bytes==0)
	{
    	//tell which client has disconnected
		printf("Client_fd %i disconnected \n", client_array[n].fd);
    	strcpy(receivedMessage,"221 Service Closing Control Connection\n");
    	sendMessageToClient(client_fd, receivedMessage); //send this message to the client
    	client_array[n].authenticated = false; //ensure the client authentication is now false as he has in a way logged out
		totalClients = totalClients - 1; //decrease the number of clients you have
		close(client_fd); //close client_fd
		return -1;
	}
	//ensure that when USER command is given, the client is not authenticated
	else if (strncmp(receivedMessage, "USER ", 5)==0 && client_array[n].authenticated==false)
	{
		char command[10];
		char username[20];
		sscanf(receivedMessage, "%s %s", command, username);
		userLoginCheck(client_fd, client_array, username, n); //then send the userLoginCheck to authenticate the client
	}
	//if the PASS command is given when the user is not authenticated then give bad sequence of commands as USER must be entered first
	else if (strncmp(receivedMessage, "PASS ", 5)==0 && client_array[n].authenticated==false)
	{
	  	sendMessageToClient(client_fd, "503 Bad Sequence of Commands\n");
	}

	// if the user has been authenticated
	else if(client_array[n].authenticated== true)
	{
		//special cases
    	if (strncmp(receivedMessage, "PASS ", 5)==0 || strncmp(receivedMessage, "USER ", 5)==0)
    	{
      		sendMessageToClient(client_fd, "503 Bad sequence of commands.\n");
   		}
   		else if(strncmp(receivedMessage, "RETR ", 5)==0) //retrieve command
		{
			int pid = fork(); //fork method

			if(pid==0)
			{
				char command[10];
				char file[20];
				sscanf(receivedMessage, "%s %s", command, file); //seperate the command and the file name
				retrieveFile(client_fd, client_array, n, file, new_sd);
			}
			else
			{
				close(new_sd);
			}

			return 0;
		}
		else if(strncmp(receivedMessage, "STOR ", 5)==0) //STOR command
        {
			int pid = fork();

			if(pid==0)
			{
				char command[10];
				char file[20];
				sscanf(receivedMessage, "%s %s", command, file); //seperate the command and the file name
				storeFile(client_fd, client_array, n, file, new_sd);
			}

			else
			{
				close(new_sd);
			}

			return 0;
        }
		else if(strcmp(receivedMessage, "LIST")==0) //LIST command
        {
        	int pid = fork();

        	if (pid == 0)
        	{
        		displayList(client_fd, client_array, n, new_sd); //displays list of files in the directory
       		}
       		else
       		{
       			close(new_sd);
       		}
   		}
		else if(strcmp(receivedMessage, "PWD")==0) //PWD command
        {
        	displayDirectory(client_fd, client_array, n); //displays the current server directory
        }
        else if(strncmp(receivedMessage, "CWD ", 4)==0) //CWD command
        {
        	char command[10];
			char directory[20];
			sscanf(receivedMessage, "%s %s", command, directory);
          	changeDirectory(client_fd, directory, client_array, n); //changes the directory
        }
		else if(strncmp(receivedMessage, "PORT ", 5)==0) // PORT command
		{
			// the user does not give this command but the client and server use this internally
			unsigned int h1,h2,h3,h4,p1,p2;
      		// retrieves the information from the Port Message sent by the Client
			sscanf(receivedMessage, "PORT %u %u %u %u %u %u", &h1,&h2,&h3,&h4,&p1,&p2);
			bzero(receivedMessage,sizeof(receivedMessage));
      		// Concatenates the h1,h2,h3,h4 values to produce the IP Address
			sprintf(receivedMessage, "%u.%u.%u.%u",h1,h2,h3,h4);
			unsigned int newdata_port = (p1*256) + p2;

			// Opening a Data Connection for FTP
			// -----------------------------------------
      		// Defines the socket structures for creating a connection
			struct sockaddr_in user_addr, target_addr;
			socklen_t addr_len = sizeof(user_addr);

			user_addr.sin_family = AF_INET;
			user_addr.sin_port = htons(6000);
			user_addr.sin_addr.s_addr = inet_addr(receivedMessage);

      		// initializing the port and IP address of the socket structure
			target_addr.sin_family = AF_INET;
			target_addr.sin_port = htons(newdata_port);
			target_addr.sin_addr.s_addr = inet_addr(receivedMessage);

      		// opening a new Socket for the Data Connection
			if( (new_sd = socket(AF_INET , SOCK_STREAM , 0)) == 0) { // Create new socket
					perror("socket creation failed\n");
					return -1;
			}

			int value  = 1;
        	setsockopt(new_sd,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value));

			// Binding the Data Port
			if(bind(new_sd, (struct sockaddr*)&user_addr,sizeof(user_addr))<0)
			{
				// If the bind fails, an error is thrown to the user
				perror("bind failed");
				exit(-1);
			}

			// Connecting with the client on the Data Connection
			if (connect(new_sd, (struct sockaddr *)&target_addr, sizeof(target_addr)) < 0) { // Connect to target
					perror("Connection Failed \n");
					close(new_sd);
					return -1;
			}
			printf("Connected to client data channel. \n");

			// Sending an Acknowledgmenet if everything works well!
			sendMessageToClient(client_fd, "200 PORT Command Successful\n");

			//internalPortCommand(client_fd, receivedMessage, new_sd);

    	}
	
		else if (strcmp(receivedMessage, "PORT") == 0) //PORT only command
		{
			//this is also an internal command
			channels++; //increments the channel
			send(client_fd,&channels,sizeof(int),0); //and sends the channels to the client
		}

    	//special cases
    	else if (strncmp(receivedMessage, "PASS ", 5)==0 || strncmp(receivedMessage, "USER ", 5)==0)
    	{
      		sendMessageToClient(client_fd, "503 Bad Sequence of Commands\n");
   		}

		// sending other commands that are not valid
		else
		{
			printf("202 Command not implemented. \n");
			sendMessageToClient(client_fd, "202 Command Not Implemented\n");
		}
	}
	// if user sends commands instead of USER or PASS before logging in
	else if (client_array[n].authenticated== false)
	{
		sendMessageToClient(client_fd, "530 Not Logged In.\n");
	}

	return 0;
}


int main()
{
	//socket
	int server_sd = socket(AF_INET,SOCK_STREAM,0);

	printf("server_sd = %d \n",server_sd);
	if(server_sd<0)
	{
		perror("socket:");
		exit(-1);
	}
	//setsock
	int value  = 1;

	// allows to reuse adress, without it one might run into "Address already in use" errors
	if(setsockopt(server_sd,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value))<0)
	{
		perror("setsock");
		return -1;
	}

	struct sockaddr_in server_addr;

	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET; //IPV4 family
	server_addr.sin_port = htons(6001); //convert port number to network-byte order
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //INADDR_ANY, INADDR_LOOP

	getcwd(serverCWD, DIRECTORY_LENGTH);

	//bind
	if(bind(server_sd, (struct sockaddr*)&server_addr,sizeof(server_addr))<0) // binds the socket to all the available network addresses which also include loopback (127.0.0.1) address
	{
		perror("bind failed");
		exit(-1);
	}
	//listen
	if(listen(server_sd,5)<0) //put socket in passive listening state and if listen call fails then print error
	{
		perror("listen failed");
		close(server_sd);
		exit(-1);
	}
	
	struct sockaddr_in client_addr; // defines client address
    bzero(&client_addr,sizeof(client_addr));
    unsigned int len = sizeof(client_addr);


	// initialize current set
    fd_set full_fdset,ready_fdset;
    // empty fd set
	FD_ZERO(&full_fdset);
	FD_SET(server_sd,&full_fdset);
    // only listening socket so it is highest
	int max_fd = server_sd;

	while(1)
	{	

		ready_fdset = full_fdset;

		if(select(max_fd+1,&ready_fdset,NULL,NULL,NULL)<0)
		{
			perror("select");
			return -1;
		}

		for(int fd = 0; fd<=max_fd; fd++)
		{
			if(FD_ISSET(fd,&ready_fdset))
			{
        		if(fd==server_sd)
				{
                    // this is a new connection
					int new_fd = accept(server_sd,NULL,NULL);
                    client_array[totalClients].fd = new_fd;
					client_array[totalClients].authenticated = false;
					printf("client fd = %d \n",new_fd);

                    // add connection to full fdset
					FD_SET(new_fd,&full_fdset);

                    // update the max_fd if new socket has higher FD
					if(new_fd>max_fd) max_fd=new_fd;
                    totalClients = totalClients + 1;

				}
                // if handling connection fails, remove fd from fdset
				else if(clientCommands(fd)==-1)
				{
					FD_CLR(fd,&full_fdset);
				}
			}
		}	
	}

	close(server_sd);
	return 0;
}
