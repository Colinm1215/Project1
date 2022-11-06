#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

sem_t read_lock;
sem_t log_lock;
sem_t write_lock;
sem_t QR_lock;

typedef struct {
	int fd;
	char *name;
} arg_t;

void getWriteLock() {
	sem_wait(&write_lock);
}

void releaseWriteLock() {
	sem_post(&write_lock);
}

void getReadLock() {
	sem_wait(&read_lock);
}

void releaseReadLock() {
	sem_post(&read_lock);
}

void getQRLock() {
	sem_wait(&QR_lock);
}

void releaseQRLock() {
	sem_post(&QR_lock);
}

void getLogLock() {
	sem_wait(&log_lock);
}

void releaseLogLock() {
	sem_post(&log_lock);
}

void *client(void *arg) {
	arg_t *args = (arg_t *) arg;
	int clientfd = args->fd;
    char *name = args->name;
    int returnVal;
    while (1) {
        returnVal = recv(clientfd, buf, 5);

        if (returnVal < 0) {
            printf("Errno from connection with client %s : %s\n", name, strerror(errno));
        }
    }
}


int main() {

    sem_init(&read_lock, 0, 1);
	const char *PORT = "2012";
	const char *msg = "Successfully connected to the server!\nType 'close' to disconnect";
        const int hostname_size = 32;
        char hostname[hostname_size];
        int BUFFER_SIZE = 255;
        char buffer[BUFFER_SIZE];
        const int backlog = 3;
        char connections[backlog][hostname_size];
        pthread_t connection_threads[backlog];
        arg_t a[backlog];
        struct addrinfo hints, *results;
        int s, sockfd, clientfd, fd;
        int max_connections = backlog;
        fd_set main_fd, read_fd;
        struct sockaddr client_addr;
        socklen_t client_len = sizeof(struct sockaddr);

        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        printf("Starting Server...\n");
        s = getaddrinfo( 0, PORT, &hints, &results);
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

        FD_ZERO(&main_fd);
        FD_SET(sockfd, &main_fd);

        while(1) {
        	read_fd = main_fd;

        	s = select(max_connections+1, &read_fd, NULL, NULL, 0);
        	if (s == -1) {
        		perror("failed");
        		exit(1);
                }

                for (fd=1; fd <= max_connections; fd++) {
                	if (FD_ISSET(fd,&read_fd)) {

                		if (fd == sockfd) {

					clientfd = accept(sockfd,
                        					&client_addr,
                        					&client_len);
        				if (clientfd == -1) {
                				perror("failed");
                				exit(1);
        				}

        				int r = getnameinfo(&client_addr, client_len, hostname, hostname_size, 0, 0, NI_NUMERICHOST);

        				strcpy(connections[clientfd], hostname);
        				a[clientfd].fd = clientfd;
        				a[clientfd].name = hostname;
        				pthread_create(&connection_threads[clientfd], NULL, client, &a[clientfd]);

        				printf("New Connection from %s\n", hostname);

        				FD_SET(clientfd, &main_fd);
        			}
        		}
        	}
       }


	for (int i = 0; i < sizeof(connections); i++) {
		close(i);
    }

    freeaddrinfo(results);

    close(sockfd);
    return(0);
}

/* going to implement the function decode_qr in server code */
char * decode_qr(unsigned int filesize, char *file){
        const char *cmd = "java -cp javase.jar:core.jar com.google.zxing.client.j2se.CommandLineRunner" ;
        /* going to use str_cpy and str_cat to concatenate the image file to command
        url = system(cmd);
         get function to grab the url from sys call */
}