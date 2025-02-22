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
#include "server.h"
//TODO: malloc ifdef for large URL buffers needs. 
//		Remove mallocs in _normal_ code.



#define PORT 5555

const char webroot[] =
"./files/";


typedef enum rest_e
{
	GET,
	POST,
	PUT,
	DELETE,
} rest_t;

//accum += sendfile(clisock, requestedFD, 0, st.st_size);
size_t mysendfile(int socket, int file, size_t offset, size_t size)
{
  size_t ret = 0;
  char* buffer = calloc(size, sizeof(char));

  ret = lseek(file, SEEK_SET, offset);
  if (ret >= 0)
  {
    memset((void*) buffer, 0, size);
    ret = read(file, (void*) buffer, size);
    if (ret >= 0)
    {
      ret = write(socket, (void*) buffer, ret);
    }
  }
  return ret;
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
  //write(fd, responseHeader, strlen(responseHeader) - 1);
  for(;;)
  {
  	sentBytes = mysendfile(socket, fd, offset, remainData);
  	remainData -= sentBytes;
  	printf("Server sent %d bytes from the file, offset now: %d, and %d remains to be send.\n", sentBytes, offset, remainData);
  	if (sentBytes <= 0 || remainData <= 0)
  		{
  			break;
  		}
  }
  // while(((sentBytes = sendfile(socket, fd, &offset, remainData)) > 0 && (remainData -= sentBytes) > 0)) {
  //   printf("Server sent %d bytes from the file, offset is now: %d, and, %d remains to be sent.\n", sentBytes, offset, remainData);
  // }
 return true; 
}



int8_t detrmineRESTtype(client_request_t cliReq)
{
	char* string = cliReq.header;
	//NO modification of string!!! going to reuse later!!!
	uint8_t ret = -1;
	switch(string[0])
	{
		case 'G':
			ret = GET;
			break;
		case 'P':
			switch(string[1])
			{
				case 'O':
					ret = POST;
					break;
				case 'U':
					ret=PUT;
					break;
				default:
					ret=-1;
			}
      break;
		case 'D':
			ret=DELETE;
			break;
	}
	return ret;
}

//Handles a newly created thread which takes a void* = fd of new client socket.
void* connectHandler(void* args) {
  // Cast to int for converstion to socket number.
  client_request_t cliRequest;
  cliRequest.clientSocket = *((int*) args);
  int clisock = cliRequest.clientSocket;
  //Generate a new buffer to hold the message from the client.
  size_t r_msg_size = MAX_BUFFER_SIZE;
  char* r_msg = (char *) malloc(r_msg_size*sizeof(char));
  //INitialize to invalid data for error detection.
  int bytesRead = -5;
  //Variable for tracking state of server, should I further process incoming, unused data.
  int returnZeroCount = 0;
    printf("New Request:\n\n");
    // error detection
    int filetype = -1;
    //Read in up to r_msg_size bytes from socket connection.
    bytesRead = read(clisock, r_msg, r_msg_size);
    //If lt 0 error encountered.
    if (bytesRead < 0) {
      printf("Error reading from socket.\n");
      //Need to free message pointer so that we don't leek.
      free(r_msg);
      //It's borken, leave.
      pthread_exit(NULL);
    } else if(bytesRead == 0) {
      //No new data to read from socket, move on.
    } else if(bytesRead < r_msg_size) {
      //Header processed move on.
      printf("%s\n", r_msg);
    } else {
      //Header is too long, if this becomes a problem increase read size, or reimplement to process with realloc.
      printf("Header toooo long, failed with too large an input.\n");
      //Cleanup heap memory.
      free(r_msg);
      pthread_exit(NULL);
    }

    cliRequest.header = r_msg;

    //What kind of header did we get?
    switch(detrmineRESTtype(cliRequest))
    {
    	case GET:
        printf("GETRequest\n");
    		handlerGETRequest(cliRequest);
			break;

  		case POST:
        printf("POSTRequest\n");
  			handlerPOSTRequest(cliRequest);
  			break;

  		case PUT:
        printf("PUTRequest\n");
  			handlerPUTRequest(cliRequest);
  			break;

  		case DELETE:
        printf("DELETERequest\n");
  			handlerDELETERequest(cliRequest);
  			break;
      default:
        printf("Error in rest type determination...\n");
  }
  //Cleanup memory, close socket, throw out thread's context.
  free(r_msg);
  close(clisock);
  pthread_exit(NULL);
}









void handleConnect(int clisock) 
{
  //New thread attribute for configuration  
  pthread_attr_t attribs;
  //new thread struct for declaring behavior
  pthread_t thread;
  //Init the thread attributes.
  pthread_attr_init(&attribs);
  //Set state so that when the thread terminates It's state and return variables are discarded
  pthread_attr_setdetachstate(&attribs, PTHREAD_CREATE_DETACHED);
  //Actuall create the thread.
  pthread_create(&thread, &attribs, connectHandler, (void*)&clisock);   
} 


int main(int argc, char* argv[]) {
  //ints to represent: socket file discriptor, new sockets generated by the OS on a request to the listening socket,
	//length of new cli socket, PID for process for easy debug lookup.
  int sockfd, newsockfd, clilen;
  int PID = getpid();
  //Structs to hold socket information for the client and server addresses.
  struct sockaddr_in cli_addr, serv_addr;
  //print PID for debug attach.
  printf("\nPID: %d\n", PID);
  //Request a new uninitialized socket from OS, if error H&CF.
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
    printf("There was an error getting a sockfd from the OS.");
    exit(-1);
  }
  //set server address to null.
  memset((void *) &serv_addr, 0, sizeof(serv_addr));
  printf("argc: %i\n", argc);
  serv_addr.sin_family = AF_INET;                 //Set address family(ipv4)
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //Set address header for any incomming address.

  short int port;								  //Used for listing connected socket on cli.
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
  listen(sockfd, 256);

  for (;;) {  //Forever
    //How big is the header for internet address.
    clilen = sizeof(cli_addr);
    // sockfd = accept(sockfd(listening), sockaddrHeaderStruct address)
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t *) &clilen);

    if (newsockfd < 0) { 
      printf("Error accepting new client.");
      //exit(-1);
      break;
    }
    //Generate a new thread to handle incoming data.
    handleConnect(newsockfd);
  }
  printf("Exited from main for some reason...\n");
}