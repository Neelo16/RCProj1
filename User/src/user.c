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

/* -------------------- Useful functions for most commands -------------------- */
void splitArgs(char *s, char **result){
	int curInd,index=0,i=0;
	for(i = 0; i < strlen(s);i++){
		if(s[i] == ' '){
			index++;
			result[index][curInd] = '\0';
			curInd = 0;
		}
		else{
			result[index][curInd] = s[i];
			curInd++;
		}
	}
}

int spaceNumber(const char *s1){
	int i,total = 0;
	for(i=0;i < strlen(s1); i++)
		if(s1[i] == ' ')
			total++;
	return total;
}

/* ----------------------- Functions for list cmd -------------------------- */
void printList(UDPHandler_p TCSHandler){
	int argLen,i;
	char **argList;
	int wordsNum = 0;
	argLen = spaceNumber(TCSHandler->buffer)+1;
	argList = (char **) malloc(sizeof(char*)*argLen);
	for(i = 0; i < argLen; i++){
		argList[i] = (char *) malloc(sizeof(char)*WORDSIZE);
	}
	splitArgs(TCSHandler->buffer,argList);
	wordsNum = atoi(argList[1]);
	puts("Languages available:");
	for(i = 0; i < wordsNum; i++)
		printf("     %d %s\n",i+1,argList[i+2]);
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
	printList(TCSHandler);
}

/* -------------------- Functions for request cmd ------------------------ */

int stringIn(const char *s1, const char *s2){
	/* Check if s1 starts with s2 */
	int i,len = strlen(s2);
	if (len > strlen(s1)){
		printf("lol what\n");
		return 0;
	}
	for(i = 0; i < len-1; i++){
		printf("%c %c\n",s1[i],s2[i]);
		if (s1[i] != s2[i])
			return 0;
	}
	return 1;
}

void request(UDPHandler_p TCSHandler,char *cmd){
	int argLen;
	char **argList;
	int N = 0;
	char c;
	char langName[WORDSIZE];
	char filename[100];
	char **words;
	int i = 0;
	int received = 0;

	argLen = spaceNumber(cmd)+1;
	if(argLen < 4){
		printf("Not enough arguments for request\n");
		return;
	}
	argList = (char **)malloc(sizeof(char *)*argLen);
	for(i = 0; i < argLen; i++){
		argList[i] = (char *) malloc(sizeof(char)*WORDSIZE);
	}
	splitArgs(cmd,argList);
	strcpy(langName,argList[1]);
	c = *argList[2];
	if(c == 'f'){
		strcpy(filename,argList[3]);
		printf("Sending request of translation from %s to portuguese of this image: %s\n",langName,filename);
	}
	else if(c == 't'){
		N = atoi(argList[3]);
		words = (char**)malloc(sizeof(char*)*N);
		for(i = 0; i < N; i++){
			words[i] = (char *) malloc(sizeof(char)*WORDSIZE);
			strcpy(words[i],argList[i+4]);
		}
		printf("Sending request of translation from %s to portuguese of %d words:\n",langName,N);
		for(i = 0; i < N; i++)
			printf("%s\n",words[i]);

		/* Send UNQ + languageName */
		received = sprintf(TCSHandler->buffer,"%s %s","UNQ",langName);
		printf("Sending: %s to TCS\n",TCSHandler->buffer);
	    if (sendto(TCSHandler->socket, TCSHandler->buffer, received , 0 , (struct sockaddr *) &TCSHandler->client, TCSHandler->clientLen) == -1)
			exitMsg("Error sending message");
		printf("Receiving message...\n");
		if ((received = recvfrom(TCSHandler->socket, TCSHandler->buffer, BUFFSIZE-1, 0, (struct sockaddr *) &TCSHandler->client, &TCSHandler->clientLen)) == -1)
			exitMsg("Error receiving messages");
		*(TCSHandler->buffer+received) = '\0';
		printf("TRS address: %s\n",TCSHandler->buffer);
		}
		else{
			printf("Invalid request\n");
		return;
	}


}
/* -------------------------------------------------------------------------------------------- */

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


	/* Repeatedly read command, execute command and print results */
	while(1){
		fgets(cmd,CMDSIZE,stdin);
		if(!strcmp(cmd,EXITCMD))
			break;
		else if(!strcmp(cmd,LISTCMD))
			list(TCSHandler);
		else if(stringIn(cmd,REQCMD))
			request(TCSHandler,cmd);
		else{
			printf("Invalid command %s\n",cmd);
		}

	}

	printf("Cleaning and exiting...\n");
    clean(TCSHandler);
	return 0;
}