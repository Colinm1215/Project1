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

sem_t log_lock;
sem_t connection_lock;
int stop = 0;
int current_connections = 0;

typedef struct {
	int fd;
	char name[];
} arg_t;

void getLogLock() {
	sem_wait(&log_lock);
}

void releaseLogLock() {
	sem_post(&log_lock);
}

void getConnectionLock() {
    sem_wait(&log_lock);
}

void releaseConnectionLock() {
    sem_post(&log_lock);
}
char* decode_qr(unsigned int filesize, char file[], char* url){
    char cmd[] = "java -cp javase.jar:core.jar com.google.zxing.client.j2se.CommandLineRunner ";
    strcat(cmd, file);

    FILE *fp;
    char path[1035];

    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Could not find or open file %s\n", file);
    }

    int parsed_result_flag = 0;
    char *parsed_url;
    int found_url = 0;
    while (fgets(path, sizeof(path), fp) != NULL && found_url == 0) {
        if (parsed_result_flag == 1) {
            parsed_url = malloc(strlen(path));
            strcpy(parsed_url, path);
            printf("Found : %s\n", parsed_url);
            found_url = 1;
        } else {
            if (strcmp(path, "Parsed result:\n") == 0) {
                parsed_result_flag = 1;
            }
        }
    }

    if (found_url == 0) {
        printf("Could not find URL\n");
    }

    pclose(fp);
    return parsed_url;
}

void *client(void *arg) {
	arg_t *args = (arg_t *) arg;
	int clientfd = args->fd;
    char *name = args->name;
    int BUFFER_SIZE = 255;
    char buffer[BUFFER_SIZE];
    int returnVal;
    printf("Name : %s\n", name);
    const char *welcome_msg = "Type 'close' to disconnect\n";

    strcpy(buffer,"Hello to ");
    strcat(buffer,name);
    strcat(buffer,"!\n");
    strcat(buffer,welcome_msg);
    returnVal = send(clientfd,buffer,strlen(buffer),0);
    buffer[returnVal] = '\0';
    if (returnVal < 0) {
        printf("Errno from connection with client %s : %s\n", name, strerror(errno));
    }

    int done = 0;

    while (done == 0) {
        returnVal = recv(clientfd, buffer, BUFFER_SIZE,0);
        buffer[returnVal] = '\0';
        if (returnVal < 0) {
            printf("Errno from connection with client %s : %s\n", name, strerror(errno));
        }
        if( strcmp(buffer,"close\n")==0 || returnVal<1) {
            if (returnVal >= 1) {
                printf("Recieved from client %s : %s", name, buffer);
            } else {
                printf("Nothing recieved from client %s\n", name);
            }
            printf("Closing Connection to %s...\n", name);
            returnVal = send(clientfd,"Closing Connection...",21,0);

            if (returnVal < 0) {
                printf("Errno from connection with client %s : %s\n", name, strerror(errno));
            }

            done = 1;
        } else {
            printf("Recieved from client %s : %s", name, buffer);
            returnVal = send(clientfd,buffer,strlen(buffer),0);

            if (returnVal < 0) {
                printf("Errno from connection with client %s : %s\n", name, strerror(errno));
            }
        }
        buffer[returnVal] = '\0';
    }
    close(clientfd);
    getConnectionLock();
    current_connections--;
    releaseConnectionLock();
    return NULL;
}

int main() {

    sem_init(&connection_lock, 0, 1);
    sem_init(&log_lock, 0, 1);
    const char *PORT = "2012";
    const int hostname_size = 32;
    char hostname[hostname_size];
    const int backlog = 3;
    char connections[backlog][hostname_size];
    pthread_t connection_threads[backlog];
    arg_t a[backlog];
    struct addrinfo hints, *results;
    int s, sockfd, clientfd, fd;
    int max_connections = backlog;
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

     printf("Listening for connections...\n");
     s = listen(sockfd, 1);
     if (s == -1) {
             perror("failed");
             exit(1);
     }

    int BUFFER_SIZE = 255;
    char buffer[BUFFER_SIZE];
    int returnVal;

     while(stop == 0) {
         for( fd=1; fd<=max_connections; fd++){
             if (fd==sockfd) {
                 getConnectionLock();
                 clientfd = accept(sockfd,
                                   &client_addr,
                                   &client_len);

                 if (clientfd == -1) {
                     perror("failed");
                     exit(1);
                 }

                 if (current_connections == max_connections) {
                     strcpy(buffer, "Error : Server Full\nDisconnecting...\n");
                     returnVal = send(clientfd,buffer,strlen(buffer),0);
                     close(clientfd);
                 } else {
                     current_connections++;

                     int r = getnameinfo(&client_addr, client_len, hostname, hostname_size, 0, 0, NI_NUMERICHOST);

                     strcpy(connections[clientfd], hostname);
                     a[clientfd].fd = clientfd;
                     char buf[hostname_size + 3];
                     snprintf(buf, hostname_size + 3, "%s(%d)\0", hostname, current_connections);
                     strcpy(a[clientfd].name, buf);
                     pthread_create(&connection_threads[clientfd], NULL, client, &a[clientfd]);

                     printf("New Connection from %s\n", a[clientfd].name);
                 }
                 releaseConnectionLock();
             }
         }
     }

     for (int fd = 1; fd <= max_connections; fd++) {
         close(fd);
     }

    close(sockfd);
    freeaddrinfo(results);

    return(0);
}