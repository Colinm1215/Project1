#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

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
        printf("1\n");

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
        printf("2\n");

        printf("Listening for connection...\n");
        s = listen(sockfd, 1);
        if (s == -1) {
                perror("failed");
                exit(1);
        }

        printf("3\n");
        client_len = sizeof(client_addr);

        clientfd = accept(sockfd,
                        &client_addr,
                        &client_len);
        if (clientfd == -1) {
                perror("failed");
                exit(1);
        }

        printf("4\n");
        printf("on file descriptor %d\n", clientfd);

        close(clientfd);

        freeaddrinfo(results);

        close(sockfd);
        return(0);
}
