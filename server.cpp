#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <iostream>
using namespace std;

#define BUFSIZE 512    //Change this to modify client input buffer size

void zombie_killer(int sig) {
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

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
    if (index < 14 || getmsg[index-10] != ' ' || getmsg[index-9] != 'H'
        || getmsg[index-8] != 'T' || getmsg[index-7] != 'T' ||
        getmsg[index-6] != 'P' || getmsg[index-5] != '/' ||
        getmsg[index-3] != '.'
        || !(getmsg[0] == 'G' && getmsg[1] == 'E'
          && getmsg[2] == 'T' && getmsg[3] == ' ' && getmsg[4] == '/')) {
        fprintf(stderr,
                "GET request improperly formated!\n");
        write(dest_socket, "HTTP/1.1 400 Bad Request HTTP\r\nContent-Length: 0\r\nContent-Type: text/html\r\n\r\n", 78);
    } else {

        char *filepath = (char*) malloc(index);
        bzero(filepath, index);
        for (int ci = 0; ci < index - 15; ci++) {
            filepath[ci] = getmsg[ci+5];
        }
        cout << "filepath1: " << filepath<<endl<<endl;
        // Take care of filenames with spaces
        char *filepath_spaces = (char*) malloc(index);
        char space_buf[4];
        bzero(filepath_spaces, index);
        bzero(space_buf, 4);
        int ci_spaces = 0;
        for (int ci = 0; ci < strlen(filepath); ci++) {
            if (ci+2 < strlen(filepath)) {
                cout << "Still less" << endl;
                space_buf[0] = filepath[ci];
                space_buf[1] = filepath[ci + 1];
                space_buf[2] = filepath[ci + 2];
                if (strcmp("%20", space_buf) == 0) {
                    cout << "HERE!!!" << endl << endl;
                    filepath_spaces[ci_spaces] = ' ';
                    ci+=2;
                    ci_spaces++;
                    continue;
                }                
            }
            cout << "filepath: " << filepath << endl;
            cout << "filepath_spaces: " << filepath_spaces << endl;
            cout << "space_buf: " << space_buf << endl;
            filepath_spaces[ci_spaces] = filepath[ci];
            ci_spaces++;
        }
        
        strcpy(filepath, filepath_spaces); // Now spaces are taken care of
        cout << "filepath2: " << filepath<<endl<<endl;
        free(filepath_spaces);
        
        int fd = open(filepath, O_RDONLY);
        if (fd == -1) {
            // Respond with 404 error
            write(dest_socket, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nContent-Type: text/html\r\n\r\n", 71);
            fprintf(stderr, "Error while opening file!\n");
        } else {
            write(dest_socket, "HTTP/1.1 200 OK\r\n", 17);

            struct stat sb;
            stat(filepath, &sb);

            char header_buf[100];
            sprintf(header_buf, "Content-Length: %lld\r\n", sb.st_size);
            write(dest_socket, header_buf, strlen(header_buf));

            // extract file type tag
            char *type_tag;
            size_t index;
            for(index = strlen(filepath); index>-1; index--) {
                if (filepath[index] == '.') break;
            }
            type_tag = (char*)malloc(strlen(filepath)-index + 1);
            bzero(type_tag, strlen(filepath)-index + 1);
            strcpy(type_tag, filepath + index + 1);

            // sort out content-type            
            if (strcmp("html", type_tag) == 0) {
                sprintf(header_buf, "Content-Type: text/html\r\n");
                cout << "this is html"<<endl<<endl<<endl;
            } else if (strcmp("jpeg", type_tag) == 0) {
                sprintf(header_buf, "Content-Type: image/jpeg\r\n");
                cout << "this is jpeg"<<endl<<endl<<endl;                
            } else if (strcmp("jpg", type_tag) == 0) {
                sprintf(header_buf, "Content-Type: image/jpeg\r\n");
                cout << "this is jpeg"<<endl<<endl<<endl;                
            } else if (strcmp("gif", type_tag) == 0) {
                sprintf(header_buf, "Content-Type: image/gif\r\n");
                cout << "this is gif"<<endl<<endl<<endl;                
            } else if (strcmp("png", type_tag) == 0) {
                sprintf(header_buf, "Content-Type: image/png\r\n");
                cout << "this is png"<<endl<<endl<<endl;                
            }
                free(type_tag);
                write(dest_socket, header_buf, strlen(header_buf));
                write(dest_socket, "\r\n", 2);
            
                char* filebuf = (char *) malloc(sizeof(char) * sb.st_size);
                read(fd, filebuf, sb.st_size);

                write(1, filebuf, sb.st_size);
                write(dest_socket, filebuf, sb.st_size);
                free(filebuf);
            }
            free(filepath);
        }
    }
int main(int argc, char **argv) {
  int pid, sockfd, newsockfd, portno;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;

  if (argc < 2) {
    fprintf(stderr,"Please provide port number\n");
    exit(1);
  }
  else
    portno = atoi(argv[1]);

  /* Register SIGCHLD handler */
  signal(SIGCHLD, zombie_killer);
  
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;          //Code of address family
  serv_addr.sin_addr.s_addr = INADDR_ANY;  //IP address of this machine
  serv_addr.sin_port = htons(portno);      //Port number in network byte order 

  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    perror("Error on binding: ");
    exit(1);
  }

  listen(sockfd, 1);
  clilen = sizeof(cli_addr);

  while (1) {
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) {
      perror("Error in accepting connection: ");
      exit(1);
    }

    pid = fork();
    if (pid < 0) {
      perror("Error forking child process: ");
      exit(1);
    }
    else if (pid == 0) { //Child process
      size_t currsize = 1024;
      char *getmsg = (char*)malloc(currsize);
      bzero(getmsg, 1024);
      char buf[BUFSIZE];
      bzero(buf, BUFSIZE);
      int n_read = read(newsockfd, buf, BUFSIZE);

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
      respond(getmsg, newsockfd);
      free(getmsg);
      close(newsockfd);
      exit(0);
    }
    else                 //Parent process
      close(newsockfd);
  }
}

