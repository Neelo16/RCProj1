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
	int received;
	/* Send User List Query */
	printf("Sending message...\n");
    if (sendto(TCSHandler->socket, SENDULQ, SENDULQSIZE , 0 , (struct sockaddr *) &TCSHandler->client, TCSHandler->clientLen) == -1)
		exitMsg("Error sending message");
	printf("Receiving message...\n");
	if ((received = recvfrom(TCSHandler->socket, TCSHandler->buffer, BUFFSIZE-1, 0, (struct sockaddr *) &TCSHandler->client, &TCSHandler->clientLen)) == -1)
		exitMsg("Error receiving messages");
	*(TCSHandler->buffer+received) = '\0';
	printf("Received message: %s\n",TCSHandler->buffer);
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

	/* Initialize UDP socket */
	memset((char *) &TCSHandler->client, 0, sizeof(TCSHandler->client));

    /* Configure UDP socket and check args for optional address or port setting */
    while((option = getopt(argc,argv,"n:p:")) != -1){
    	switch (option){
    		case 'p':
    			defaultP = 0;
    			TCSHandler->client.sin_port = htons(atoi(optarg));
    			printf("Using port: %s\n",optarg);
    			break;
    		case 'n':
    			defaultA = 0;
    			printf("Using address: %s\n",optarg);
    			inet_aton(optarg , &TCSHandler->client.sin_addr);
    			break;
    		case '?':
		        fprintf (stderr, "Unknown option `-%c'.\n", optopt);
    	}
    }
    if(defaultP){
    	printf("Using default port: %d\n",DEFAULTPORT);
    	TCSHandler->client.sin_port = htons(DEFAULTPORT);
    }
   	if(defaultA){
   		printf("Using default addr: %s\n",DEFAULTADDR);
		inet_aton(DEFAULTADDR , &TCSHandler->client.sin_addr);
   	}
	TCSHandler->client.sin_family = AF_INET;


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