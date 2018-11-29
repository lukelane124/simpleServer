#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/sendfile.h>
#include <sys/stat.h>




// TODO: Some string functions here are not thread safe, these need to be updated to either their thread safe counterparts, or to be used with a global mutex.


#define PORT 5555
#define MAX_BUFFER_SIZE 1024

typedef enum supported_f_s
{
  HTML_FILE,
  PNG_FILE,
  BINARY_FILE,
  LUA_FILE,
}supported_f_t;
const char webpage[] =
"HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n<!DOCTYPE html>\r\n<html><head><title>%s</title></head><body>%s</body></html>\r\n\r\n\0";
const char timepage[] =
"HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n<!DOCTYPE html>\r\n<html><head><title>Default</title></head><body><h1>This is the default file for Tommy's Server 0.0.1</h1><br><h2>%s</h2></body></html>\r\n\r\n\0";
const char fofPage[] =
"HTTP/1.1 404 Not Found\r\nConnection: close\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n<!DOCTYPE html>\r\n<html><head><title>404 Not Found</title></head><body><h1>Error 404</h1><br>File not found. Closing connection</body></html>\r\n\r\n\0";
const char htmlHeader[] =
"HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html; \r\n\r\n";
const char pngHeader[] =
"HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: image/png\r\nContent-Length: %i\r\n\r\n";
const char textHeader[] =
"HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/text; \r\n\r\n";

int extensionHash(char* str) {
  if (strncmp(str, ".html", 5) == 0){
    printf("HTML_FILE\n");
    return HTML_FILE;
  } else if (strncmp(str, ".png", 4) == 0){
    printf("PNG_FILE\n");
    return PNG_FILE;
  } else if (strncmp(str, ".bin", 4) == 0) {
    printf("BINARY_FILE\n");
    return BINARY_FILE;
  } else if (strncmp(str, ".lua", 4) == 0){
    printf("LUA_FILE\n");
    return LUA_FILE;
  } else {
    return -1;
  }
}

bool sendFileOverSocket(int fd, int socket, const char* formatHeader) {
  struct stat st;
  fstat(fd, &st);
  char responseHeader[sizeof(formatHeader) + 120] = {0};
  sprintf(responseHeader, formatHeader, st.st_size);
  printf("FORMAT HEADER:\n%s\n", responseHeader);
  int sentBytes = 0;
  long int offset = 0;
  int remainData = st.st_size;
  while(((sentBytes = sendfile(socket, fd, &offset, remainData)) > 0 && (remainData -= sentBytes) > 0)) {
    printf("Server sent %d bytes from the file, offset is now: %d, and, %d remains to be sent.\n", sentBytes, offset, remainData);
  }
 return true; 
}

void* connectHandler(void* args) {
  int clisock = *((int*) args);
  size_t r_msg_size = MAX_BUFFER_SIZE;
  char* r_msg = (char *) malloc(r_msg_size*sizeof(char));
  int bytesRead = -5;
  bool cont = true;
  int returnZeroCount = 0;
  whileloop:
  while(cont) 
  {
    printf("New Request:\n\n");
    int filetype = -1;
    bytesRead = read(clisock, r_msg, r_msg_size);
    if (bytesRead < 0) {
      printf("Error reading from socket.\n");
      free(r_msg);
      pthread_exit(NULL);
    } else if(bytesRead == 0) {
      if (returnZeroCount > 20) 
        cont = false;
      sleep(.3);   
      returnZeroCount++;
      continue;
    } else if(bytesRead < r_msg_size) {
      printf("%s\n", r_msg);
    } else {
      printf("Header toooo long, failed with too large an input.\n");
      exit(-3);
    }

    char str[256] = {0};
    char* file;
    int count = sscanf(r_msg, "GET %s %*s\n", &str);
    printf("GET header: \n%s\n", str);
    int requestedFD = 0;
     if(strcmp("/", str) == 0) {
      printf("root file requested\n");
      time_t t = time(NULL);
      struct tm *tm = localtime(&t);
      char s[64] = {0};
      strftime(s, sizeof(s), "%c", tm);
      char dateTime[512] = {0};
      sprintf(dateTime, timepage, s);
      printf("TimePage%s\n", dateTime);
      write(clisock, dateTime, sizeof(dateTime)-1);
      memset(r_msg, 0, r_msg_size);
      memset(str, 0, 256);
      goto whileloop;
    } 
    else 
    {
      char delim[] =  {"?#"};
      char* saveptr;
      file = strtok_r(str, delim, &saveptr);
      char fileName[256] = {0};
      for (int i = 1; (file[i-1] != '\0')&&(i < 257); i++) {
        fileName[i-1]=file[i];
      }
      requestedFD = open(fileName, O_RDONLY);
      char* extension = strrchr(fileName, '.');
      char* kvp = strtok_r(0, delim, &saveptr);
      printf("requested file:%s\nKVPtoken: %s\nFileExtension: %s\n",fileName, kvp, extension);
      if (requestedFD == -1)
      {
        printf("File Not found..\n");
        write(clisock, fofPage, sizeof(fofPage)-1);
        pthread_exit(NULL);
      }
      struct stat st;
      fstat(requestedFD, &st);
      if (extensionHash(extension) != HTML_FILE) 
      {
        switch(extensionHash(extension)) 
        {
          case PNG_FILE:{
              //write(clisock, pngHeader, sizeof(pngHeader)-1);

            //bool sendFileOverSocket(int fd, FILE* socket, char* formatHeader) {
            sendFileOverSocket(requestedFD, clisock, pngHeader);
            printf("PNG file requested\n");
            break;
          }
          default:{}
        }
      } else {
        write(clisock, htmlHeader, sizeof(htmlHeader)-1);
      }

      long unsigned int accum = 0;
      while (accum < st.st_size) {
        accum += sendfile(clisock, requestedFD, 0, st.st_size);
        printf("Writing with Acum Value: %li\n", accum);
      }
      char name[60]={0};
      sprintf(name, "/proc/self/fd/%i", requestedFD);
      char name2[60]={0};
      readlink(name, name2, 60);
      printf("File being sent over socket: %s\n", name2);
       // close(requestedFD);
       // printf("Thread closing\n");
        //goto end;
    }

  }
  end:
  free(r_msg);
  close(clisock);
  pthread_exit(NULL);
}

void handleConnect(int clisock) {
  
  pthread_attr_t attribs;
  pthread_t thread;
  pthread_attr_init(&attribs);
  pthread_attr_setdetachstate(&attribs, PTHREAD_CREATE_DETACHED);
  pthread_create(&thread, &attribs, connectHandler, (void*)&clisock);   
} 



int main(int argc, char* argv[]) {
  int sockfd, newsockfd, clilen;
  struct sockaddr_in cli_addr, serv_addr;
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
    printf("There was an error getting a sockfd from the OS.");
    exit(-1);
  }
  memset((void *) &serv_addr, 0, sizeof(serv_addr));
  printf("argc: %i\n", argc);
  serv_addr.sin_family = AF_INET;                 //Set address family(ipv4)
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //Set address header for any incomming address.

  short int port;
  if (argc > 1) {
    port = atoi(argv[1]);
    serv_addr.sin_port = htons(port);
  }else { 
    serv_addr.sin_port = htons(PORT);               //Set listening port.
    port = PORT;
  }
  

  //bind our new "listening socket" with the params set above.
  while (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) { 
    printf("Error binding Sock to addr.\n");
    sleep(10);
    //exit(-1);
  }
  printf("Binding to listeing port %i completed successfully\n", port);
  //Tell OS we would like to start listening on this socket, we're not going to create a connection to a specific address.
  //  We want to keep this open so that we can be available for anyone who wants to talk.
  //Second param is "backlog" of connections the OS will pool for us.
  listen(sockfd, 10);

  for (;;) {  //Forever
    //How big is the header for internet address.
    clilen = sizeof(cli_addr);
    // sockfd = accept(sockfd(listening), sockaddrHeaderStruct address)
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t *) &clilen);

    if (newsockfd < 0) { 
      printf("Error accepting new client.");
      //exit(-1);
    }
    handleConnect(newsockfd);
  }
}