#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

int main() {

        struct addrinfo hints, *results;
        int s, sockfd, clientfd;
        struct sockaddr client_addr;
        socklen_t client_len;

        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        printf("Starting Server...\n");
        s = getaddrinfo( 0, "2012", &hints, &results);
        if ( s != 0 ) {
                perror("failed");
                exit(1);
        }

        sockfd = socket(
                        results->ai_family,
                        results->ai_socktype,
                        results->ai_protocol);
        if (sockfd == -1) {
                perror("failed");
                exit(1);
        }

        s = bind(sockfd,
                        results->ai_addr,
                        results->ai_addrlen);
        if (s == -1) {
                perror("failed");
                exit(0);
        }

        printf("Listening for connection...\n");
        s = listen(sockfd, 1);
        if (s == -1) {
                perror("failed");
                exit(1);
        }

        client_len = sizeof(client_addr);

        clientfd = accept(sockfd,
                        &client_addr,
                        &client_len);
        if (clientfd == -1) {
                perror("failed");
                exit(1);
        }
        printf("Connected to Client\n");

        char buf[5];

        memset(buf, '\0', sizeof(buf));

        int returnVal = 1;

        returnVal = read(clientfd, buf, 6);

        if (returnVal < 0) {
                printf("Errno : %s\n", strerror(errno));
        }

        printf("Client Message : %s\n", buf);

        close(clientfd);

        freeaddrinfo(results);

        close(sockfd);
        return(0);
}

int decode_qr(){

}

