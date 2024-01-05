#include<stdio.h> // header contaning functions such as printf
#include<string.h> // header containing functions such as strcmp
#include<dirent.h> // header containing functions to work with directories
#include<unistd.h> // header containing fork() and unix standard functions, also for unix specific functions declarations : fork(), getpid(), getppid()
#include<stdlib.h> // header containing functions such as malloc, free, atoi, exit

#define SIZE 256
#define DIR_SIZE 100

// function to change directory
void change_directory(char* dir) {
	// use chhdir to change directory
	// error if chdir fails
	if (chdir(dir) == -1) {
		printf("550 No such file or directory.\n");
	}

	else
	{
		// print message that directory has been changed
		char new_directory[DIR_SIZE];
		bzero(new_directory, sizeof(new_directory));
		getcwd(new_directory, DIR_SIZE);
		char changed[SIZE] = "Client directory changed to: ";
		strcat(changed, new_directory);
		printf("%s\n", changed);
	}
}

// function to print directory contents
void print_directory_contents(char* name) {
	// open directory
	DIR* directory;
	struct dirent *dir;
	directory = opendir(name);

	// print all files in the directory
	if (directory) {
		while((dir=readdir(directory))!=NULL) {
			// do not print if directory name is "." or "..", we don't need these
			if(strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") ==0)
				continue;
			printf("%s\n",dir->d_name);
		}

		// close directory
		closedir(directory);
	}

	else {
		printf("!LIST error");
		exit(-1);
	}
}