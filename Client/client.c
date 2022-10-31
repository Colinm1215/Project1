#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

int main() {

        struct addrinfo hints, server;
        char IP[] = "10.63.18.1";
        char Port[] = "8066";
        int r, sockfd;

        printf("Client Starting,,,");
        memset($hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        r = getaddrinfo( IP, Port, &hints, &server );
        if ( r != 0 ) {
                perror("failed");
                exit(1);
        }
        puts("done");
}
