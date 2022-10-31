#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

int main() {

        struct addrinfo hints, *results;
        int s;

        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        s = getaddrinfo(NULL, "2012", &hints, &results);
        if ( s != 0 ) {
                perror("failed");
                exit(1);
        }
}
