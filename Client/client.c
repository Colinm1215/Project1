#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

int main() {

        struct addrinfo hints, *server;
        char IP[] = "0";
        char Port[] = "2012";
        int r, sockfd;

        printf("Client Starting...\n");
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        r = getaddrinfo( 0, "2012", &hints, &server);
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

        printf("Sending Test Text Message to Server : Hello\n");

        char buf[5] = "Hello";

        int returnVal = write(sockfd, buf, sizeof(buf));

        printf("Return Val %d\n", returnVal);
        freeaddrinfo(server);
}
