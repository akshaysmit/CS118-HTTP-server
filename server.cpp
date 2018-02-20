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

#define BUFSIZE 2048    //Change this to modify client input buffer size
char err_msg[]=  "<!doctype HTML>\n\
<html>\n<head><title> 404 File Not Found</title></head>\n\n\
<body><h1> 404 File not found.</h1><p> The requested file was not found. Please check the filename and try again.</p></body>\n\
</html>\n ";

void zombie_killer(int sig) { //This is to reap child processes after they're done
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

// getmsg should be a full GET HTML message
void respond(char* getmsg, int dest_socket) {
  char doublebuff[2];
  doublebuff[0] = getmsg[0];
  doublebuff[1] = getmsg[1];
  size_t index;
    
  for(index = 0; !(doublebuff[0] == '\r' && doublebuff[1] == '\n');
      index++) {
    doublebuff[0] = getmsg[index];
    doublebuff[1] = getmsg[index + 1];
  }
 
  // Here index will point to index of '\r' in first occurance of "\r\n"
  if (index < 14 || getmsg[index-10] != ' ' || getmsg[index-9] != 'H'
      || getmsg[index-8] != 'T' || getmsg[index-7] != 'T' ||
      getmsg[index-6] != 'P' || getmsg[index-5] != '/' ||
      getmsg[index-3] != '.'
      || !(getmsg[0] == 'G' && getmsg[1] == 'E'
	   && getmsg[2] == 'T' && getmsg[3] == ' ' && getmsg[4] == '/')) {
    fprintf(stderr,
	    "GET request improperly formated!\n");
    write(dest_socket, "HTTP/1.1 400 Bad Request\r\n\r\n", 28);
    return;
  }
  
  else {                 //extract filename
    char *filepath = (char*) malloc(index+5);
    bzero(filepath, index);
    for (int ci = 0; ci < index - 15; ci++) {
      filepath[ci] = getmsg[ci+5];
    }

    //take care of spaces
    int len = strlen(filepath);
    int j = 0;
    for (int i = 0; i < len; i++) {
      if (i+2 < len && filepath[i] == '%' && filepath[i+1] == '2' && filepath[i+2] == '0') {
	filepath[j] = ' ';
	i = i+2;
      }
      else
	filepath[j] = filepath[i];
      
      j++;
    }
    filepath[j] ='\0';

    //convert to lowercase before extracting extension
    char *ext = strstr(filepath, ".");
    if (ext != NULL) {
      while(ext != filepath + strlen(filepath)) {
	*ext = tolower(*ext);
	ext++;
      }
    }
    
    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
      // Respond with 404 error
      write(dest_socket, "HTTP/1.1 404 Not Found\r\n\r\n", 26);
      write(dest_socket, err_msg, strlen(err_msg));
      fprintf(stderr, "Error while opening file!\n");
      return;
    }
    
    else {
      write(dest_socket, "HTTP/1.1 200 OK\r\n", 17);
      
      struct stat sb;
      stat(filepath, &sb);
      
      char header_buf[200];
      sprintf(header_buf, "Content-Length: %ld\r\n", sb.st_size);
      write(dest_socket, header_buf, strlen(header_buf));
 
      // extract file extension
      if (strstr(filepath, ".html") != NULL) {
	sprintf(header_buf, "Content-Type: text/html\r\n");
      }
      else if (strstr(filepath, ".htm") != NULL) {
	sprintf(header_buf, "Content-Type: text/htm\r\n");
      }
      else if (strstr(filepath, ".jpeg") != NULL) {
	sprintf(header_buf, "Content-Type: image/jpeg\r\n");
      }
      else if (strstr(filepath, ".jpg") != NULL) {
	sprintf(header_buf, "Content-Type: image/jpg\r\n");
      }
      else if (strstr(filepath, ".gif") != NULL) {
	sprintf(header_buf, "Content-Type: image/gif\r\n");
      }
      else if (strstr(filepath, ".png") != NULL) {
	sprintf(header_buf, "Content-Type: image/png\r\n");
      }
      else if (strstr(filepath, ".pdf") != NULL) {
        sprintf(header_buf, "Content-Type: application/pdf\r\n");
      }
      else {
	sprintf(header_buf, "Content-Type: application/octet-stream\r\n");
      }
      
      write(dest_socket, header_buf, strlen(header_buf));
      write(dest_socket, "\r\n", 2);
      char* filebuf = (char *) malloc(sizeof(char) * sb.st_size);
      read(fd, filebuf, sb.st_size);
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
      char buf[BUFSIZE];
      bzero(buf, BUFSIZE);
      int n_read = read(newsockfd, buf, BUFSIZE);
      if (n_read < 0) {
	fprintf(stderr, "Error reading from socket\n");
	exit(1);
      }
      else if (n_read == 0)
	exit(0);
      
      write(1, buf, n_read);     //dump to stdout
      respond(buf, newsockfd);
      close(newsockfd);
      exit(0);
    }
    else                         //Parent process
      close(newsockfd);
  }
}

