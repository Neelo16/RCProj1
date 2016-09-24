/* TODO MAKE CODE MORE BEAUTIFUL. FILE TRANSFER. FIX int 2147483647+1 BUGS */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>l
#include "user.h"

void exitMsg(const char *msg){
	perror(msg);
	exit(1);
}

void cleanUDP(UDPHandler_p handler){
	close(handler->socket);
	free(handler);
}
void cleanTCP(TCPHandler_p handler){
	if(handler->connected)
		close(handler->clientFD);
	free(handler);
}

void cleanLanguagesList(char **languages,int langNumber){
	int i;
	for(i = 0; i < langNumber; i++)
		free(languages[i]);
	free(languages);
}

/* ----------------------- Functions for list cmd -------------------------- */
int getLanguages(UDPHandler_p TCSHandler, char ***languages){
	char *part;
	int langNumber;
	int i;

	part = strtok(TCSHandler->buffer," ");
	part = strtok(NULL," ");
	langNumber = atoi(part);
	*languages = (char **) malloc(sizeof(char *)*langNumber);
	for(i = 0; i < langNumber; i++){
		part = strtok(NULL, " ");
		*languages[i] = (char *) malloc(sizeof(char)*WORDSIZE);
		strcpy(*languages[i],part);
	}

	return langNumber;
}

int list(UDPHandler_p TCSHandler, char ***languages){
	int received;
	int langNumber;
	int i;
	/* Send User List Query */
	printf("Sending message...\n");
    if (sendto(TCSHandler->socket, SENDULQ, SENDULQSIZE , 0 , (struct sockaddr *) &TCSHandler->client, TCSHandler->clientLen) == -1)
		exitMsg("Error sending message");
	printf("Receiving message...\n");
	if ((received = recvfrom(TCSHandler->socket, TCSHandler->buffer, BUFFSIZE-1, 0, (struct sockaddr *) &TCSHandler->client, &TCSHandler->clientLen)) == -1)
		exitMsg("Error receiving messages");
	*(TCSHandler->buffer+received) = '\0';
	langNumber = getLanguages(TCSHandler,languages);

	/* Print languages */
	puts("Languages available:");
	for(i = 0; i < langNumber; i++)
		printf(" %d- %s\n",i+1,*languages[i]);
	return langNumber;
}

/* -------------------- Functions for request cmd ------------------------ */

int stringIn(const char *s1, const char *s2){
	/* Check if s1 starts with s2 */
	int i,len = strlen(s2);

	if (len > strlen(s1))
		return 0;
	for(i = 0; i < len-1; i++)
		if (s1[i] != s2[i])
			return 0;
	
	return 1;
}

void request(UDPHandler_p TCSHandler,char *cmd, char **languages){
	int i = 0,received = 0;
	int langName,N=0;
	char filename[100];
	char **words;
	char *part,c,*ip;
	unsigned int port;

	part = strtok(cmd," ");
	part = strtok(NULL," ");
	langName = atoi(part);
	if((part = strtok(NULL," ")) == NULL) {
		printf("Not enough arguments for request\n");
		return;
	}
	c = *part;
	if(c == 'f'){
		part = strtok(NULL, " ");
		strcpy(filename,part);
		printf("Sending request of translation from %s to portuguese of this image: %s\n",languages[langName-1],filename);
	}
	else if(c == 't'){
		if((part = strtok(NULL," ")) == NULL) {
			printf("Not enough arguments for request\n");
			return;
		}
		N = atoi(part);
		words = (char**)malloc(sizeof(char*)*N);
		for(i = 0; i < N; i++){
			words[i] = (char *) malloc(sizeof(char)*WORDSIZE);
			if((part = strtok(NULL," ")) == NULL) {
				printf("Not enough arguments for request\n");
				return;
			}
			strcpy(words[i],part);
		}

		/* Send UNQ + languageName */
		received = sprintf(TCSHandler->buffer,"%s %d","UNQ",langName);
		printf("Sending: %s to TCS\n",TCSHandler->buffer);
	    if (sendto(TCSHandler->socket, TCSHandler->buffer, received , 0 , (struct sockaddr *) &TCSHandler->client, TCSHandler->clientLen) == -1)
			exitMsg("Error sending message");
		printf("Receiving message...\n");
		
		/* Receive UNR */
		if ((received = recvfrom(TCSHandler->socket, TCSHandler->buffer, BUFFSIZE-1, 0, (struct sockaddr *) &TCSHandler->client, &TCSHandler->clientLen)) == -1)
			exitMsg("Error receiving messages");
		*(TCSHandler->buffer+received) = '\0';
		printf("TRS address: %s\n",TCSHandler->buffer);

		part = strtok(TCSHandler->buffer, " ");
		if(strcmp(part,"UNR")){
			printf("Error. Received %s from TCS server\n",TCSHandler->buffer);
			return;
		}
		part = strtok(NULL," ");
		if(strcmp(part,languages[langName])){
			printf("Error. You asked to translate %s but TCS sent you the %s TRS translator",languages[langName],part);
			return;
		}
		ip = strtok(NULL," ");
		if(ip == NULL){
			printf("Didnt receive enough data from TCS: %s\n",TCSHandler->buffer);
			return;
		}
		part = strtok(NULL, " ");
		if (part == NULL){
			printf("Didnt receive enough data from TCS: %s\n",TCSHandler->buffer);
			return;
		}			
		port = atoi(part);
		TCPConnection(TRSHandler, ip, port, languages[langName]);
		/* Free some resources */
		for(i = 0; i < N; i++)
			free(words[i]);
		free(words);
		}
	else{
		printf("Invalid request\n");
		return;
	}

}
/* -------------------------------------------------------------------------------------------- */

void TCPConnection(TCPHandler_p TRSHandler, const char *ip, const int port, const char *language){
	/* Estabilishes a TCP connection with the TRS server */
	if(TRSHandler->language != NULL)
		close(TRSHandler->clientFD);

	if ((TRSHandler->clientFD = socket(AF_INET, SOCK_STREAM,0)) == -1)
		exitMsg("Error creating TCP socket");

	/* Configure settings of the TCP socket */
	TRSHandler->server.sin_family = AF_INET;
	TRSHandler->serverSize = sizeof(TRSHandler->server);
	TRSHandler->server.sin_addr.s_addr = inet_addr(ip);
	TRSHandler->server.sin_port = htons(port);
	memset(TRSHandler->server.sin_zero, '\0', sizeof TRSHandler->server.sin_zero);

	strcpy(TRSHandler->language,language);

}

int main(int argc, char **argv){
	
	char cmd[CMDSIZE];
	char option;
	unsigned short int defaultP=1,defaultA=1;
	UDPHandler_p TCSHandler;
	TCPHandler_p TRSHandler;
	char **languages = NULL; /* Hold the known languages */
	int langNumber = 0; /* Number of languages being hold */

	/* Create TCP socket co communicate with the TRS's */
	TRSHandler = (TCPHandler_p) malloc(sizeof(struct TCPHandler));
	TRSHandler->connected = 0;
	
	/* Create UDP socket to communicate with TCS */
	TCSHandler = (UDPHandler_p) malloc(sizeof(struct UDPHandler));
	TCSHandler->clientLen = sizeof(TCSHandler->client);
	if ( (TCSHandler->socket = socket(AF_INET, SOCK_DGRAM,0)) == -1)
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


	/* Repeatedly read command, execute command and print results */
	while(1){
		fgets(cmd,CMDSIZE,stdin);
		if(!strcmp(cmd,EXITCMD))
			break;
		else if(!strcmp(cmd,LISTCMD)){
			if(langNumber)
				cleanLanguagesList(languages,langNumber);
			languages = NULL;
			langNumber = list(TCSHandler,&languages);
		}
		else if(stringIn(cmd,REQCMD))
			request(TCSHandler,cmd,languages);
		else{
			printf("Invalid command %s\n",cmd);
		}

	}

	printf("Cleaning and exiting...\n");
	if(langNumber)
		cleanLanguagesList(languages,langNumber);
    cleanUDP(TCSHandler);
    cleanTCP(TRSHandler);
	return 0;
}