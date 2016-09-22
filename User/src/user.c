#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "user.h"

void exitMsg(const char *msg){
	perror(msg);
	exit(1);
}

void clean(UDPHandler_p handler){
	close(handler->socket);
	free(handler);
}

void list(UDPHandler_p TCSHandler){
	/* Send User List Query */
    if (sendto(TCSHandler->socket, SENDULQ, SENDULQSIZE , 0 , (struct sockaddr *) &TCSHandler->client, TCSHandler->clientLen) == -1){
		exitMsg("Error sending message");
	}
}

int main(int argc, char **argv){
	
	char cmd[CMDSIZE];
	char option;
	unsigned short int defaultP=1,defaultA=1;
	UDPHandler_p TCSHandler;

	/* Create UDP socket to communicate with TCS */
	TCSHandler = (UDPHandler_p) malloc(sizeof(struct UDPHandler));
	TCSHandler->clientLen = sizeof(TCSHandler->client);
	if ( (TCSHandler->socket = socket(AF_INET,SOCK_DGRAM,0)) == -1)
		exitMsg("Error creating UDP socket");

	/* Configure UDP socket */
	memset((char *) &TCSHandler->client, 0, sizeof(TCSHandler->client));
    TCSHandler->client.sin_family = AF_INET;

    /* Check args for optional address or port setting */
    while((option = getopt(argc,argv,"n:p")) != -1){
    	switch (option){
    		case 'n':
    			defaultP = 0;
    			TCSHandler->client.sin_port = htons(atoi(optarg));
    			printf("Using port: %c",*optarg);
    			break;
    		case 'p':
    			defaultA = 0;
    			inet_aton(optarg , &TCSHandler->client.sin_addr);
    			break;
    		case '?':
		        fprintf (stderr, "Unknown option `-%c'.\n", optopt);
    	}
    }
    if(defaultP)
    	TCSHandler->client.sin_port = htons(DEFAULTPORT);
   	if(defaultA)
		inet_aton(DEFAULTADDR , &TCSHandler->client.sin_addr);

	/* Repeatly read command, execute command and print results */
	while(1){
		scanf("%s",cmd);
		if(!strcmp(cmd,EXITCMD))
			break;
		else if(!strcmp(cmd,LISTCMD))
			list(TCSHandler);
		else{
			printf("Invalid command\n");
		}
	}

	printf("Cleaning and exiting...\n");
    clean(TCSHandler);
	return 0;
}