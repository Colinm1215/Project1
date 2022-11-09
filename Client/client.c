#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#define SIZE 1024

// sendFile is used to parse through the data contained in the loaded file
// to send to the server -- it reads it in segments
void sendFile(FILE *file, int sockfd){

	char data_segment[SIZE] = "";

	while(fgets(data_segment, SIZE, file) != NULL){
		if(send(sockfd, data_segment, sizeof(data_segment), 0) == -1){
			perror("Error: unable to send file.");
			exit(1);
		}
		bzero(data_segment, SIZE);
	}
}

int main(int argc, char *argv[]) {

	char ip[20];
	char port[6];
	
	if (argc > 3){
                printf("Error: too many arguments.\n");
        } else if(argc == 1 || argc == 2){
                printf("Please enter the IP and Port.\n");
        } else if (argc == 3){
                strcpy(ip, argv[1]);
		strcpy(port, argv[2]);
        } else {
                printf("Error: please try again.\n");
        }

	printf("IP: %s, Port: %s\n\n", ip, port);
	
	struct addrinfo hints, *server;
	int r, sockfd;
	
	FILE* qr_file = NULL;
	char buffer[50] = "";
	
	printf("Client Starting...\n");
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	r = getaddrinfo(ip, port, &hints, &server);
	if ( r != 0 ) {
		perror("failed");
		exit(0);
	}

	sockfd = socket(
			server->ai_family,
			server->ai_socktype,
			server->ai_protocol);
	if (sockfd == -1) {
		perror("failed");
		exit(0);
	}

	r = connect(sockfd,
			server->ai_addr,
			server->ai_addrlen);
	if (r == -1) {
		perror("failed");
		exit(1);
	}

	printf("Enter a file to be decoded by the server: \n");
	scanf("%s",buffer);
	
	qr_file = fopen(buffer,"r");
	
	if(qr_file==NULL){
		printf("File not found.");
		exit(1);
	}

	printf("File opened: %s\n\n", buffer);

	printf("Sending '%s' File to Server...\n",buffer);
	
	sendFile(qr_file, sockfd);
	printf("File has been sent successfully!\n");

	fclose(qr_file);


/*
	printf("Sending Test Text Message to Server : Hello\n");
	
 	char buf[5] = "Hello";

	int returnVal = write(sockfd, buf, sizeof(buf));

	printf("Return Val %d\n", returnVal);
*/
	freeaddrinfo(server);

}
