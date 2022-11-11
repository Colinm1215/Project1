CS 3516 Project 1 README
Lauren Flanagan and Colin Mettler
11/10/22

Cited Resources:
- Prof. Taneja's CS 3516 Lecture Slides
- https://man7.org/linux/man-pages/man3/system.3.html
- https://aticleworld.com/socket-programming-in-c-using-tcpip/#:~:text=Create%20a%20socket%20with%20the,calling%20the%20close()%20function.
- https://man7.org/linux/man-pages/man3/getaddrinfo.3.html
- https://idiotdeveloper.com/file-transfer-using-tcp-socket-in-c/
- https://stackoverflow.com/questions/7637765/why-fd-set-fd-zero-for-select-inside-of-loop
- https://stackoverflow.com/questions/646241/c-run-a-system-command-and-get-output
- https://www.tutorialspoint.com/getopt-function-in-c-to-parse-command-line-arguments
- https://randomqr.com/
- https://www.geeksforgeeks.org/socket-programming-cc/
- https://www.geeksforgeeks.org/c-program-find-size-file/
- https://idiotdeveloper.com/file-transfer-using-tcp-socket-in-c/
- https://manpages.org/fopen
- https://stackoverflow.com/questions/646241/c-run-a-system-command-and-get-output

Server:
To run the server.c file, first use the 'make all' command to make the file.
Then the to use it run as ./server [-p port] [-t timeout(s)] [-m max_users] [-r number_of_requests] [-s rate_of_requests(s)]

where in :
-p port is the port of the server
-t timeout(s) is the time in seconds a client must be idle to timeout
-m max_users is the maximum connections at a time
and 
-r number_of_requests 
and
-s rate_of_requests(s)
is the number of requests a client can make per the time specified in seconds

Client:

To run the client.c file, first use the 'make all' command to make the file.
Then, run the command './client [IP] [Port]', including the IP address and
Port as arguments separated by a space (**Please make sure to put them
in this order, IP then Port, to be able to run the client**).

At any time when a client is running and looking for user input, you can use
the command 'close' to end a specific client connection and the command
'shutdown' to end all connections and close the server connection.
