/******************************************************************************
 * CS 118, Winter 2018 - Project 1
 * -------------------------------
 * Authors: 
 *     - Akshay Smit
 *     - Nikola Samardzic
 * 
 *****************************************************************************/

#include <sys/stat.h>
#include <fcntl.h>

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

#include <iostream>
using namespace std;

// getmsg should be a full GET HTML message
void respond(char* getmsg, int dest_socket) {
    cout<< "getmsg: " << getmsg;
    char doublebuff[2];
    doublebuff[0] = getmsg[0];
    doublebuff[1] = getmsg[1];
    size_t index;
    for(index = 0; !(doublebuff[0] == '\r' && doublebuff[1] == '\n');
        index++) {
        doublebuff[0] = getmsg[index];
        doublebuff[1] = getmsg[index + 1];
    }
    cout << "index: " << index << endl << endl;
    // Here index will point to index of '\r' in first occurance of "\r\n"
    if (!(getmsg[0] == 'G' && getmsg[1] == 'E'
          && getmsg[2] == 'T' && getmsg[3] == ' ' && getmsg[4] == '/')) {
        fprintf(stderr,
                "GET request does not start with keyword GET!\n");
        exit(1);
    }

    char *filepath = (char*) malloc(index);
    bzero(filepath, index);
    for (int ci = 0; ci < index - 15; ci++) {
        cout << ci+5 << " :: " << getmsg[ci+5] << endl;
        filepath[ci] = getmsg[ci+5];
    }
    cout<< "filepath: " << filepath << "END" << endl;                      
    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Error while opening file!\n");        
        exit(1);
    }
    
    write(dest_socket, "HTTP/1.1 200 OK\r\n\r\n", 23);

    char c;    
    for (read(fd, &c, 1); read(fd, &c, 1) > 0; )
        write(dest_socket, &c, 1);
    write(dest_socket, "\r\n\r\n", 4);
}

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
                    size_t currsize = 1024;
                    char *getmsg = (char*)malloc(currsize);
                    bzero(getmsg, 1024);
                    char buf[BUFSIZE];
                    bzero(buf, BUFSIZE);
                    int n_read = read(i, buf, BUFSIZE);
                    write(1, buf, n_read); //Echo output
                    size_t len = strlen(getmsg);
                    for (size_t index = 0; index < n_read; index++) {
                        getmsg[len] = buf[index];
                        len++;
                        if (len >= currsize) {
                            currsize*=2;
                            char *getmsg_realloc = (char*)realloc(getmsg, currsize);
                            if (getmsg_realloc) {
                                getmsg = getmsg_realloc;
                            } else {
                                fprintf(stderr,
                                        "Memory allocation error!\n");
                                exit(1);
                            }
                        }
                    }
                    bzero(buf, BUFSIZE);
                    respond(getmsg, i);
                }
            }
        }
    }
}

