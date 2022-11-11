#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#define SIZE 1024

// getSize calculates size of file being sent
long int getSize(char filename[]){

    FILE* file = fopen(filename, "r");

    // check file
    if (file == NULL) {
        printf("Cannot find file specified.\n");
        return -1;
    }

    fseek(file, 0L, SEEK_END);
    long int size = ftell(file);
    fclose(file);

    return(size);
}

// sendFile is used to parse through the data contained in the loaded file
// to send to the server -- it reads it in segments
void sendFile(char filename[], int sockfd){

    FILE* qr_file = NULL;
    long int size = getSize(filename);
    char data_segment[size];
    memset(data_segment, 0, size);

    qr_file = fopen(filename,"r");

    if(qr_file==NULL){
        printf("File not found.");
        exit(1);
    }

    printf("File opened: %s\n\n", filename);

    int r = 1;

    printf("Sending '%s' File to Server...\n",filename);

    while((r = fread(data_segment, size, 1, qr_file)) == 1) {
        if (send(sockfd, data_segment, sizeof(data_segment), 0) == -1) {
            perror("Error in sending file.");
            exit(1);
        }
        bzero(data_segment, SIZE);
    }

    fclose(qr_file);

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

    char buffy[100] = "";

    int returnVal = recv(sockfd, buffy, 100, 0);
    buffy[returnVal] = '\0';
    printf("%s\n", buffy);

    while(1){

        printf("Enter a file to be decoded by the server: \n");
        scanf("%s",buffer);

        // if client wants to close connection
        if (strcmp(buffer, "close") == 0) {
            puts("Closing connection...");
            returnVal = send(sockfd, buffer, strlen(buffer), 0);
            if (returnVal < 0) {
                printf("Errno from connection with server: %s\n", strerror(errno));
            }
            break;
        }

        // if client wants to shut down the server
        if (strcmp(buffer, "shutdown") == 0) {
            puts("Shutting down server...");
            returnVal = send(sockfd, buffer, strlen(buffer), 0);
            if (returnVal < 0) {
                printf("Errno from connection with server: %s\n", strerror(errno));
            }
            break;
        }

        // get file size
        long int file_size_int = getSize(buffer);
        printf("File size: %ld\n", file_size_int);
        if (file_size_int > 0) {

            // convert file size to char arr
            char size_buffer[10] = "";
            sprintf(size_buffer, "%ld", file_size_int);

            // send file size to server
            if (send(sockfd, size_buffer, sizeof(size_buffer), 0) == -1) {
                perror("Error in sending file.");
                exit(1);
            }

            int returnVal = 0;
            char msg_buffer[100] = "";

            // check that server received file size
            returnVal = recv(sockfd, msg_buffer, 100, 0);
            printf("%s\n", msg_buffer);

            char msg_cmp[100] = "Downloading file!\n";

            if (strcmp(msg_buffer, msg_cmp) == 0) {
                sendFile(buffer, sockfd);
                printf("File has been sent successfully!\n\n");
                printf("Waiting for URL from server...\n");
                char val_buf[10] = "";
                char sz_buf[100] = "";
                char url_buf[100] = "";
                returnVal = recv(sockfd, val_buf, 10, 0);
                int retVal = atoi(val_buf);
                returnVal = recv(sockfd, sz_buf, 100, 0);
                returnVal = recv(sockfd, url_buf, atoi(sz_buf), 0);
                if (retVal == 0) {
                    printf("URL : %s\n", url_buf);
                } else {
                    printf("Error Occurred\n");
                }
            } else {
                printf("Aborting file send.\n");
            }
        }

    }

    // close socket
    close(sockfd);

    freeaddrinfo(server);

}
