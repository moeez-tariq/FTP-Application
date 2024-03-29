# FTP Protocol Implementation

This project is an implementation of a simplified version of the FTP (File Transfer Protocol) application protocol. It consists of two separate programs: an FTP server and an FTP client. The FTP server is responsible for maintaining FTP sessions and providing file access, while the FTP client is split into two components - an FTP user interface and an FTP client to make requests to the FTP server.

## Project Overview

The primary goal of this project is to create a basic FTP system, allowing users to interact with a server to perform file transfers and manage remote files. The system is designed to support concurrent connections, enabling multiple clients to interact with the server simultaneously. Key features and components of the project include:

### FTP Server
The server is responsible for handling incoming FTP client connections, maintaining FTP sessions, and providing file access to clients. It supports concurrent connections, allowing multiple clients to interact with it simultaneously.

### FTP Client
The FTP client is divided into two components:

FTP User Interface: This component provides a simple command-line interface for users to interact with the FTP server. Users can input commands to perform various operations, such as uploading, downloading, listing files, and navigating directories on the server.

FTP Client Core: The core client component communicates with the FTP server, sending requests and receiving responses. It manages the control connection with the server and handles data transfer using select() and fork() as appropriate.

## How to Run

Follow these steps to run the project:

1. In the main directory, execute the command `makefile` to build the necessary components.
2. To start the server, navigate to the `server_directory` and run `./server`.
3. To start the client, navigate to the `client_directory` and run `./client`.

## FTP Commands

1. **USER username:** Identifies the user trying to login to the FTP server. Used for authentication.

2. **PASS password:** Authenticates the user with the provided password.

3. **STOR filename:** Uploads a local file named filename from the current client directory to the current server directory.

4. **RETR filename:** Downloads a file named filename from the current server directory to the current client directory.

5. **LIST:** Lists all files under the current server directory.

6. **!LIST:** Lists all files under the current client directory.

7. **CWD foldername:** Changes the current server directory.

8. **!CWD foldername:** Changes the current client directory.

9. **PWD:** Displays the current server directory.

10. **!PWD:** Displays the current client directory.

11. **QUIT:** Quits the FTP session and closes the control TCP connection.

## Academic Use Notice

Kindly be aware that this project was created as part of an educational assignment within a course offered by NYU Abu Dhabi's Computer Science department. It is intended for educational and learning purposes. Using this code for academic submissions or assignments is strictly prohibited. I encourage students to use this repository as a learning resource and to write their own code for assignments to uphold academic integrity.