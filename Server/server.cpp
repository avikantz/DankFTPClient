#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <iostream>

#include "../common.hpp"

void send_file (int sockfd) {

	// printf("Client requested file: \n");
	char s_name[NAME_SIZE];
	int bytes = recv(sockfd, s_name, NAME_SIZE, 0);
	char path[PATH_SIZE];

	if (bytes > 0) {
		printf("Client requested file: %s", s_name);
		strcpy(path, "./Files/");
		strcat(path, s_name);  
	} else {
		return;
	}

	int fd = open(path, O_RDONLY);
	char buff[BUFFER_SIZE + 1];

	if(fd == -1) {

		printf("File could not be opened :(.");
		header_block head;
		head.error_code = 1;

	} else {

		header_block head;
		head.is_req = 0;
		head.is_resp = 1;

		off_t filesize = lseek(fd, 0, SEEK_END);
		printf("\nFile size: %lld, transferring...\n", filesize);

		head.filesize = filesize;

		lseek(fd, 0, SEEK_SET);
		head.error_code = 0;
		send(sockfd, (header_block *)&head, sizeof(head), 0);

		int nob;
		while((nob = read(fd, buff, BUFFER_SIZE)) > 0) {
			send(sockfd, buff, nob, 0);
		};

		printf("Transferred!\n");
		close(fd);
	}
}

void receive_file(int sockfd) {
	char filename[NAME_SIZE];
	recv(sockfd, filename, NAME_SIZE, 0);
	char path[PATH_SIZE];
	printf("File name: %s\n", filename);
	strcpy(path, "./Files/");
	strcat(path, filename);

	printf("%s\n", path);
	char buff[BUFFER_SIZE + 1];
	int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	
	if(fd == -1) {
		printf("Error in receiving...\n");
		return;
	}
	int nob;
	while((nob = recv(sockfd, buff, BUFFER_SIZE, 0)) > 0) {
		write(fd, buff, nob);
		if(nob < BUFFER_SIZE){
			printf("Done\n");
			break;
		}
			
	}
	close(fd);
	printf("File received.\n");
}

void send_listing(int sockfd) {
	printf("Sending listing\n");
	system("ls ./Files/ > listing.txt");
	int fd = open("./listing.txt", O_RDONLY);
	if(fd == -1) {
		printf("Error fetching the songlist\n");
		return;
	}
	char buff[BUFFER_SIZE + 1];
	int nob;
	while((nob = read(fd, buff, BUFFER_SIZE)) > 0) {
		send(sockfd, buff, nob, 0);
	}
}


void handle_client(int cli_sockfd) {
	printf("Client connected to the server...\n");

	while (YES) {
		_control ctrl;
		printf("Waiting for command...\n");
		int bytes = recv(cli_sockfd, (_control *)&ctrl, sizeof(ctrl), 0);
		if (bytes > 0) {
			if (ctrl.is_error) {
				printf("Error on client side.\n");
				break;
			} else {
				switch(ctrl.command) {
					case -1: 
						std::cout << "Closing client connection.";
						return;
					case REQ_FILE: send_file(cli_sockfd);
						break;
					case REQ_LIST: send_listing(cli_sockfd);
						break;
					case FUPLOAD: receive_file(cli_sockfd);
						break;
				}
			}    
		} else {
			std::cerr << "Connection dropped by client" << std::endl;
			exit(-1);
		}

	}
}

int main (int argc, char *argv[]) {

	char *prt = (char *)malloc(20 * sizeof(char));
	if (argc < 2) {
		printf("Please specify port: ");
		scanf(" %s", prt);
	} else {
		prt = argv[1];
	}
	int port = atoi(prt);

	int sockfd;
	sockaddr_in_t server, client;
	sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = INADDR_ANY;

	fflush(stdout);
	printf("Binding server...\n");

	bind(sockfd, (struct sockaddr *)&server, sizeof(server));

	listen(sockfd, 5);
	socklen_t clen = sizeof(client);
	int newsockfd;

	printf("Server active at: %d\n", port);

	while (YES) {
		newsockfd = accept(sockfd, (struct sockaddr *)&client, &clen);
		pid_t pid = fork();
		if(pid == 0) {
			handle_client(newsockfd);
			close(newsockfd);
			exit(1);
		}
	}
	close(sockfd);

	return 0;
}
