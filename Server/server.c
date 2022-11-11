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
#include <stdint.h>
#include <time.h>

#define SUCCESS 0
#define FAILURE 1
#define TIMEOUT 2
#define RATEEXCEED 3
#define MAX_SIZE_FILE 5000

sem_t log_lock;
sem_t connection_lock;
int stop = 0;
int server_shutdown = 0;
int current_connections = 0;

typedef struct {
    int fd;
    struct timeval tv;
    int num_reqs;
    int per_sec;
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

void logger(char *msg, char *ip){

    //get log lock function
    getLogLock();

    FILE *f;
    //append message to log file
    f = fopen("project1.log", "a");
    if (f == NULL) {
        printf("-----Log Error-----\n");
    }

    // cited source: https://stackoverflow.com/questions/1442116/how-to-get-the-date-and-time-values-in-a-c-program
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    //printf("now: %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    if(ip==NULL){
        fprintf(f, "%d-%02d-%02d %02d:%02d:%02d %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, msg);
    } else if(ip!=NULL){
        fprintf(f, "%d-%02d-%02d %02d:%02d:%02d %s %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, ip, msg);
    }

    //release log lock function
    releaseLogLock();
}

char *decode_qr(unsigned int filesize, char file[], int *returnValue) {
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
            found_url = 1;
        } else {
            if (strcmp(path, "Parsed result:\n") == 0) {
                parsed_result_flag = 1;
            }
        }
    }

    if (found_url == 0) {
        printf("Could not find URL\n");
        *returnValue = FAILURE;
    }

    pclose(fp);
    return parsed_url;
}

char *process_file(int sockfd, long SIZE, int initSec){
    int n;
    FILE *fp;
    char *filename = "tmp.png";
    char buffer[SIZE];

    fp = fopen(filename, "w");

    fd_set main_set, working_set;
    FD_ZERO(&main_set);
    FD_SET(sockfd, &main_set);

    int bytes_remaining = SIZE;

    struct timeval tv;
    tv.tv_sec = initSec;
    tv.tv_usec = 0;

    working_set = main_set;

    int selectVal = select(sockfd + 1, &working_set, NULL, NULL, &tv);

    if (FD_ISSET(sockfd, &working_set)) {

        while (1) {

            tv.tv_sec = 0;
            tv.tv_usec = 0;

            working_set = main_set;

            int selectVal = select(sockfd + 1, &working_set, NULL, NULL, &tv);

            if (selectVal <= 0) break;

            if (FD_ISSET(sockfd, &working_set)) {

                if (bytes_remaining > 0) {

                    n = recv(sockfd, buffer, bytes_remaining, 0);

                    bytes_remaining -= n;

                    fwrite(buffer, sizeof(buffer), 1, fp);
                } else {
                    if (selectVal > 0) {
                        n = recv(sockfd, buffer, SIZE, 0);
                    }
                }
                bzero(buffer, SIZE);
            } else {
                break;
            }
        }
    }
    FD_CLR(sockfd, &main_set);
    fclose(fp);
    return filename;
}

int eval_req(int num_reqs_per_time, int time_limit, time_t *prev_req_arr) {
    int eval = 0;
    time_t cur_time;
    time_t temp_array[num_reqs_per_time];
    memcpy(&temp_array, prev_req_arr, sizeof(time_t)*num_reqs_per_time+1);
    time(&cur_time);

    int i;
    for (i = 0; i < num_reqs_per_time; i++) {
        if (temp_array[i] == 0) break;
    }

    if (i < num_reqs_per_time) {
        temp_array[i] = cur_time;
        eval = 1;
    } else {
        time_t since_oldest = cur_time - temp_array[0];

        if (since_oldest > time_limit) {
            eval = 1;
        } else {
            for (int j = 0; j < num_reqs_per_time; j++) {
                if (j < num_reqs_per_time-1) {
                    temp_array[j] = temp_array[j + 1];
                } else {
                    temp_array[j] = cur_time;
                }
            }
        }
    }

    memcpy(prev_req_arr, &temp_array, sizeof(time_t)*num_reqs_per_time+1);

    return eval;
}

void *client(void *arg) {

    arg_t *args = (arg_t *) arg;
    int myfd = args->fd;
    char *name = args->name;
    time_t num_requests_in_time[args->num_reqs];
    memset(num_requests_in_time, 0, sizeof(time_t)*args->num_reqs);
    int BUFFER_SIZE = 255;
    char buffer[BUFFER_SIZE];
    int returnVal;
    const char *welcome_msg = "Type 'close' to disconnect\nType 'shutdown' to shutdown the server\n";
    int done = 0;

    strcpy(buffer, "Hello to ");
    strcat(buffer, name);
    strcat(buffer, "!\n");
    strcat(buffer, welcome_msg);
    returnVal = send(myfd, buffer, strlen(buffer), 0);

    if (returnVal < 0) {
        printf("Errno from connection with client %s : %s\n", name, strerror(errno));
        done = 1;
    }

    fd_set main_set, working_set;
    FD_ZERO(&main_set);
    FD_SET(myfd, &main_set);

    int processing_file = 0;
    int file_size = 0;
    char file_buffer[MAX_SIZE_FILE];
    int bytes_read_so_far = 0;
    int oversize_flag = 0;
    int file_downloaded_flag = 0;

    while (done == 0) {
        memset(buffer, 0, BUFFER_SIZE);

        if (stop == 1) {
            char *msg = "Server shutdown!\nClosing Connection...\n";
            returnVal = send(myfd, msg, strlen(msg), 0);
            break;
        }

        struct timeval tv;
        tv.tv_sec = args->tv.tv_sec;
        tv.tv_usec = args->tv.tv_usec;

        working_set = main_set;

        int selectVal = select(myfd + 1, &working_set, NULL, NULL, &tv);

        if (selectVal == 1) {
            if (FD_ISSET(myfd, &working_set)) {
                returnVal = recv(myfd, buffer, BUFFER_SIZE, 0);
                if (returnVal < 0) {
                    printf("Errno from connection with client %s : %s\n", name, strerror(errno));
                    continue;
                }

                if (eval_req(args->num_reqs, args->per_sec, num_requests_in_time) == 0) {
                    //TODO: change to include %d references
                    char *decline_msg = "Too many requests!\nMaximum of %d requests per user per %d seconds!\n";
                    send(myfd, decline_msg, strlen(decline_msg), 0);
                    continue;
                }

                buffer[returnVal] = '\0';

                if (strcmp(buffer, "close") == 0 || returnVal < 1) {
                    if (returnVal >= 1) {
                        printf("Received from client %s : %s\n", name, buffer);
                    } else {
                        printf("Nothing received from client %s\n", name);
                    }
                    printf("Closing Connection to %s...\n", name);
                    returnVal = send(myfd, "Closing Connection...\n", 21, 0);

                    if (returnVal < 0) {
                        printf("Errno from connection with client %s : %s\n", name, strerror(errno));
                    }

                    done = 1;
                } else if( strcmp(buffer,"shutdown")==0 ) {
                    printf("Shutdown command received from %s.\nClosing all connections...", name);
                    char *msg = "Shutdown command received.\nClosing Connection...\n";
                    returnVal = send(myfd, msg, strlen(msg), 0);

                    if (returnVal < 0) {
                        printf("Errno from connection with client %s : %s\n", name, strerror(errno));
                    }
                    stop = 1;
                    done = 1;
                } else {
                    char *err_str;
                    long file_size = strtol(buffer, &err_str, 10);
                    if (file_size > 0 && strcmp(err_str, "") == 0) {
                        if(file_size <= MAX_SIZE_FILE){
                            processing_file = 1;
                            char *return_msg = "Downloading file!\n";
                            returnVal = send(myfd, return_msg, strlen(return_msg), 0);
                            printf("Downloading file of size %ld from client %s\n", file_size, name);
                            char *tmp_filename = process_file(myfd, file_size, args->tv.tv_sec);
                            int val = 0;
                            printf("Processing file from %s\n", name);
                            char *url = decode_qr(file_size, tmp_filename, &val);
                            printf("Sending processed URL %s to client %s\n", url, name);
                            char val_buf[10] = "";
                            sprintf(val_buf,"%d", val);
                            char sz_buf[100] = "";
                            sprintf(sz_buf,"%d", strlen(url));
                            returnVal = send(myfd, val_buf, 10, 0);
                            returnVal = send(myfd, sz_buf, 100, 0);
                            returnVal = send(myfd, url, strlen(url), 0);
                        } else if(file_size > MAX_SIZE_FILE){
                            printf("Max File Size Exceeded. Please try again.\n");
                            char *return_msg = "Max File Size Exceeded. Please try again.\n";
                            returnVal = send(myfd, return_msg, strlen(return_msg), 0);
                        }
                    } else {
                        printf("File Download Error\n");
                        char *return_msg = "File Download Error\n";
                        returnVal = send(myfd, return_msg, strlen(return_msg), 0);
                    }
                }
            }
        } else if (selectVal == -1) {
            printf("Select() error\n");
        } else {
            printf("Client %s idle for %ld seconds, timeout occurred.\nClosing Connection...\n", name, args->tv.tv_sec);
            char *timeout_msg = "Client idle for too long, timeout occurred.\nClosing Connection...\n";
            returnVal = send(myfd, timeout_msg, strlen(timeout_msg), 0);

            done = 1;
        }
    }

    close(myfd);
    getConnectionLock();
    current_connections--;
    releaseConnectionLock();
    return NULL;
}

int main(int argc, char *argv[]) {

    const char *arg_err = "Usage: %s [-p port] [-t timeout(s)] [-m max_users] [-r number_of_requests] [-s rate_of_requests(s)]\n";

    int max_connections = 3;
    int max_requests_per_user = 3;
    int max_requests_per_user_per_second = 60;
    char *PORT = "2012";
    int timeout_time = 80;
    int opt;

    int rate_flag = 0;

    while ((opt = getopt(argc, argv, "t:p:m:r:s:")) != -1) {
        switch (opt) {
            case 'p':
                PORT = optarg;
                break;
            case 't':
                timeout_time = atoi(optarg);
                break;
            case 'm':
                max_connections = atoi(optarg);
                break;
            case 'r':
                max_requests_per_user = atoi(optarg);
                rate_flag++;
                break;
            case 's':
                max_requests_per_user_per_second = atoi(optarg);
                rate_flag++;
                break;
            default:
                fprintf(stderr, arg_err,
                        argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (rate_flag != 2 && rate_flag != 0) {
        fprintf(stderr, arg_err,
                argv[0]);
        exit(EXIT_FAILURE);
    }

    sem_init(&connection_lock, 0, 1);
    sem_init(&log_lock, 0, 1);
    const int hostname_size = 32;
    char hostname[hostname_size];
    char connections[max_connections][hostname_size];
    pthread_t connection_threads[max_connections];
    arg_t a[max_connections];
    struct addrinfo hints, *results;
    int s, sockfd, clientfd, fd;
    struct sockaddr client_addr;
    socklen_t client_len = sizeof(struct sockaddr);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    printf("Starting Server...\n");
    logger("Starting Server.\n",NULL); //no IP address necessary
    s = getaddrinfo(0, PORT, &hints, &results);
    if (s != 0) {
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
    s = listen(sockfd, max_connections);
    if (s == -1) {
        perror("failed");
        exit(1);
    }

    int BUFFER_SIZE = 255;
    char buffer[BUFFER_SIZE];
    int returnVal;
    fd_set main_fd, read_fd;

    FD_ZERO(&main_fd);
    FD_SET(sockfd, &main_fd);

    while (stop == 0) {
        read_fd = main_fd;

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        int selectVal = select(sockfd + 1, &read_fd, NULL, NULL, &tv);

        if (FD_ISSET(sockfd, &read_fd) && selectVal == 1) {
            printf("Searching for new connections...\n");
            int clientfd = accept(
                    sockfd,
                    (struct sockaddr *)&client_addr,
                    &client_len
            );

            if (clientfd == -1) {
                perror("failed");
                exit(1);
            }

            int r = getnameinfo(
                    (struct sockaddr *)&client_addr,
                    client_len,
                    hostname,
                    hostname_size,
                    0,
                    0,
                    NI_NUMERICHOST
            );

            if (returnVal < 0) {
                printf("Errno from connection with client %s : %s\n", hostname, strerror(errno));
            }

            getConnectionLock();
            if (current_connections == max_connections) {
                releaseConnectionLock();
                strcpy(buffer, "Error : Server Full\nDisconnecting...\n");
                returnVal = send(clientfd, buffer, strlen(buffer), 0);

                if (returnVal < 0) {
                    printf("Errno from connection with client %s : %s\n", hostname, strerror(errno));
                }
                close(clientfd);
            } else {
                current_connections++;
                releaseConnectionLock();

                int index = clientfd-sockfd-1;

                strcpy(connections[index],hostname);
                a[index].fd = clientfd;
                a[index].tv.tv_sec = timeout_time;
                a[index].tv.tv_usec = 0;
                a[index].num_reqs = max_requests_per_user;
                a[index].per_sec = max_requests_per_user_per_second;
                strcpy(a[index].name, hostname);
                pthread_create(&connection_threads[index], NULL, client, &a[index]);

                printf("New Connection from %s\n", a[index].name);

            }
        } else if (selectVal == -1) {
            perror("failed");
            exit(1);
        }
    }
    FD_CLR(sockfd, &main_fd);
    FD_CLR(sockfd, &read_fd);

    printf("Shutting down server\n");
    printf("Waiting for all clients to disconnect\n");
    while (current_connections > 0) {}
    printf("All clients disconnected, goodbye\n");

    close(sockfd);
    freeaddrinfo(results);

    return (0);
}
