/******************************************************************************
 * CS 118, Winter 2018 - Project 1
 * -------------------------------
 * Authors: 
 *     - Akshay Smit
 *     - Nikola Samardzic
 * 
 *****************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>

#define BUFSIZE 512    //Change this to modify client input buffer size

int main(int argc, char **argv) {
    int sockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 2) {
        fprintf(stderr,"Please provide port number\n");
        exit(1);
    }
    else
        portno = atoi(argv[1]);
  
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;          //Code of address family
    serv_addr.sin_addr.s_addr = INADDR_ANY;  //IP address of this machine
    serv_addr.sin_port = htons(portno);      //Port number in network byte order 

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error on binding: ");
        exit(1);
    }

    fd_set active_fd_set, read_fd_set;
    listen(sockfd, 1);
 
    /* Initialize active socket set */
    FD_ZERO(&active_fd_set);
    FD_SET(sockfd, &active_fd_set);
  
    while (1) {
        read_fd_set = active_fd_set;
        /* Wait until a socket is ready */
        if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
            perror("Error with select: ");
            exit(1);
        }

        /* Check sockets ready with input */
        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &read_fd_set)) {

                /* New connection request */
                if (i == sockfd) {
                    int newfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
                    if (newfd < 0) {
                        perror("Error accepting connection: ");
                        exit(1);
                    }

                    FD_SET(newfd, &active_fd_set);
                }

                /* Data from connected client */
                else {
                    char buf[BUFSIZE]; 
                    int n_read = read(i, buf, BUFSIZE);
                    write(1, buf, n_read); //Echo output
                    
                    //HTTP RESPONSE GOES HERE
                    //YOU MUST WRITE TO FILE DESCRIPTOR i
                    //CLIENT INPUT IS IN THE BUFFER buf
                }
            }
        }
    }
}

