#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "server.h"

continueHeader[] =
"HTTP/1.1 100 continue\r\n\r\n";
					break;

/*
int clientSocket;
	char* header;*/
void handlerPUTRequest(client_request_t cliReq)
{
	char* r_msg = cliReq.header;
	int clisock = cliReq.clientSocket;
	size_t msg_offset = 0;
	int fd;
	int nElementsRead = -1;
	unsigned long fileLength = -1;
	char str[MAX_BUFFER_SIZE];
	int count = sscanf(r_msg, "PUT %s %*s\n", &str);
	char delim[] =  {"?#"};
	char* saveptr;
	char* file;
	char* readBuffer;

	file = strtok_r(str, delim, &saveptr);
	msg_offset = strlen(file)+2;
	char fileName[256] = {0};
	for (int i = 1; (file[i-1] != '\0')&&(i < 257); i++) 
	{
        fileName[i-1]=file[i];
    }

    printf("r_msg: %s\n", r_msg);
    saveptr = NULL;
    r_msg = strtok_r(r_msg, "\n", &saveptr);
    count = 0;
    int mtCount = 0;
    while(sscanf(r_msg, "Content-Length: %ul  %*s", &fileLength) == 0)
    {

    	if(count++ > 30 || fileLength != -1)// || mtCount >= 2)
    	{
    		break;
    	}
    	r_msg = strtok_r(NULL, "\r\n", &saveptr);
    	if (r_msg == NULL)
    	{
    		break;
    	}
    }

    printf("The file length is: %d\n", fileLength);
    printf("Num elements successfully read %d\n", nElementsRead );
    fd = open(fileName, O_RDONLY);
    if (fd < 0)
    {
    	strerror(errno);
    }
   if(fileLength == -1 && fd > 0)
	{
		fileLength = MAX_BUFFER_SIZE;
	}
	
	mtCount = 0;
	count = 0
	if (fd > 0)
	{
		readBuffer = calloc(fileLength,sizeof(char));

	    if (readBuffer == NULL)
		{
			fd = -1;
			printf("%s", strerror(errno));
		}
		else{
			while((count += read(clisock, (void*) readBuffer, fileLength)) == 0)
			{
				if (count < 0)
				{
					printf("read error on client socket. Exit\n");
				}
				else if(count == 0 && mtCount>0)
				{
					write(clisock, continueHeader, strlen(continueHeader)-1);
					mtCount++;
					continue;//break;
				}
				else if(count == )
			}
		}
	}
	else
	{//return 40X error to server

	}

	close(fd);
	free(readBuffer);
	

}
