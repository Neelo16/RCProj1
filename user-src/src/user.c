/* TODO MAKE CODE MORE BEAUTIFUL. FILE TRANSFER. FIX int 2147483647+1 BUGS */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "user.h"
#include <errno.h>

void exitMsg(const char *msg){
	perror(msg);
	exit(1);
}

void cleanUDP(UDPHandler_p handler){
	close(handler->socket);
	free(handler);
}

void cleanLanguagesList(char **languages,int langNumber){
	int i;
	for(i = 0; i < langNumber; i++)
		free(languages[i]);
	free(languages);
}

int safeSendUDP(UDPHandler_p TCSHandler, const char *toSend, unsigned int toSendLen){
	int received,tries = 0;
	while(tries < 3){
		if (sendto(TCSHandler->socket, toSend, toSendLen , 0 , (struct sockaddr *) &TCSHandler->client, TCSHandler->clientLen) == -1)
			exitMsg("Error sending message");
		if ((received = recvfrom(TCSHandler->socket, TCSHandler->buffer, BUFFSIZE-1, 0, (struct sockaddr *) &TCSHandler->client, &TCSHandler->clientLen)) == -1){
			if(errno != ETIMEDOUT && errno != EAGAIN) /* FIXME */
				exitMsg("Error receiving messages");
		}
		else
			break;
		tries++;
	}
	if(tries < 3){
		*(TCSHandler->buffer+received) = '\0';
		return 1;
	}
	return 0;
}

/* ----------------------- Functions for list cmd -------------------------- */
int getLanguages(UDPHandler_p TCSHandler, char ***languages){
	char *part;
	int langNumber;
	int i;
	/* ULR 3 linguagem1 linguagem2 linguagem3 */

	part = strtok(TCSHandler->buffer," ");
	part = strtok(NULL," ");
	langNumber = atoi(part);
	*languages = (char **) malloc(sizeof(char *)*langNumber);
	for(i = 0; i < langNumber; i++)
		(*languages)[i] = (char *) malloc(sizeof(char)*WORDSIZE);
	for(i = 0; i < langNumber; i++){
		part = strtok(NULL, " ");
		strcpy((*languages)[i],part);
	}
	return langNumber;
}

int list(UDPHandler_p TCSHandler, char ***languages){
	int langNumber;
	int i;

	/* Send User List Query */
	if(!safeSendUDP(TCSHandler,SENDULQ,SENDULQSIZE))
		return 0; /* FIXME */
	langNumber = getLanguages(TCSHandler,languages);

	if(!langNumber)
		printf("No languages available\n");

	/* Print languages */
	for(i = 0; i < langNumber; i++)
		printf(" %d- %s\n",i+1,(*languages)[i]);
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

int parseTCSUNR(UDPHandler_p TCSHandler, char **ip, unsigned int *port){
	char *part;
	part = strtok(TCSHandler->buffer, " ");
	if(part == NULL || strcmp(part,"UNR")){
		printf("Error. Received %s from TCS server\n",TCSHandler->buffer);
		return 0;
	}
	part = strtok(NULL," ");
	if(part == NULL){
		printf("Didnt receive enough data from TCS: %s\n",TCSHandler->buffer);
		return 0;
	}
	*ip = strtok(NULL," ");
	if(*ip == NULL){
		printf("Didnt receive enough data from TCS: %s\n",TCSHandler->buffer);
		return 0;
	}
	part = strtok(NULL, " ");
	if (part == NULL){
		printf("Didnt receive enough data from TCS: %s\n",TCSHandler->buffer);
		return 0;
	}	
	*port = atoi(part);
	return 1;
}

void request(UDPHandler_p TCSHandler,TCPHandler_p TRSHandler, char *cmd, char **languages, int langNumber){
	int i = 0,received = 0;
	int langName,N=0;
	char filename[100];
	char **words;
	char *part,c,*ip;
	unsigned int port;
	long int size = 0;
	int good = 0;
	FILE *file;

	part = strtok(cmd," ");
	part = strtok(NULL," ");
	langName = atoi(part)-1;
	if((part = strtok(NULL," ")) == NULL) {
		printf("Not enough arguments for request\n");
		return;
	}
	c = *part;
	if(c == 'f'){
		part = strtok(NULL, " ");
		if(part == NULL){
			printf("Not enough arguments for request\n");
			return;
		}
		strcpy(filename,part);
		*(filename+strlen(filename)-1) = '\0';
		printf("Sending request of translation from %s to portuguese of this image: %s\n",languages[langName],filename);
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
	}
	else{
		printf("Invalid request\n");
		return;
	}
	
	while(!good){

		if(strcmp(TRSHandler->language,languages[langName])){

			/* Send UNQ + languageName */
			received = sprintf(TCSHandler->buffer,"%s %s","UNQ",languages[langName]);

			if(!safeSendUDP(TCSHandler,TCSHandler->buffer,received))
				return; /* FIXME */

			if(!parseTCSUNR(TCSHandler,&ip, &port)) return;
			good = TCPConnection(TRSHandler, ip, port, languages[langName]);
		}
		else{
			good = !connect(TRSHandler->clientFD, (struct sockaddr *) &TRSHandler->server, TRSHandler->serverSize);
			if(!good)
				memset(TRSHandler->language,0,strlen(TRSHandler->language));
		}


	}

	if(c == 't'){
		received = sprintf(TRSHandler->buffer, "%s %c %d","TRQ",'t',N);
		for(i = 0; i < N; i++)
			received += sprintf(TRSHandler->buffer+received," %s",words[i]);
		write(TRSHandler->clientFD,TRSHandler->buffer,received);
		received = read(TRSHandler->clientFD,TRSHandler->buffer,BUFFSIZE-1);
		*(TRSHandler->buffer+received) = '\0';

		/* Free some resources */
		for(i = 0; i < N; i++)
			free(words[i]);
		free(words);

		printf(" %s:",ip);
		part = strtok(TRSHandler->buffer," ");
		part = strtok(NULL," ");
		part = strtok(NULL," ");
		N = atoi(part);
		for(i = 0; i < N; i++){
			part = strtok(NULL," ");
			if(part == NULL){
				printf("TRS said he would send %d words but he only sent %d",N,i);
				return;
			}
			printf(" %s",part);
		}
		puts("");
	}
	else if(c == 'f'){
		file = fopen(filename,"rb");
		if(!file){
			printf("File %s does not exist\n",filename);
			return;
		}

		fseek(file, 0L, SEEK_END);
		received = sprintf(TRSHandler->buffer, "%s %c %s %ld ","TRQ",'f',filename,ftell(file));
		printf("     %ld Bytes to transmit\n",ftell(file));
		rewind(file);
		write(TRSHandler->clientFD,TRSHandler->buffer,received);

		i = 0;
		while((c = getc(file)) != EOF){
			*(TRSHandler->buffer+i) = c;
			if(i == BUFFSIZE-1){
				i = 0;
				write(TRSHandler->clientFD,TRSHandler->buffer,BUFFSIZE);
			}
			else
				i++;
		}
		if(i > 0)
			write(TRSHandler->clientFD,TRSHandler->buffer,i);
		c = '\n';
		write(TRSHandler->clientFD,&c,1);
		fclose(file);

		i = 0;
		read(TRSHandler->clientFD,TRSHandler->buffer,6);/*TRR f*/
		while(1){
			read(TRSHandler->clientFD,&c,1);/*filename */
			if(c == ' ')
				break;
			*(filename+i) = c;
			i++;
		}
		*(filename+i) = '\0';

		file = fopen(filename,"wb");
		i = 0;
		while(1){
			i += read(TRSHandler->clientFD,TRSHandler->buffer+i,1); /*size */
			if(*(TRSHandler->buffer+i) == ' ')
				break;
		}
		*(TRSHandler->buffer+i+1) = '\0';
		size = atoi(TRSHandler->buffer);
		if(file != NULL){
			while(1){
				received = read(TRSHandler->clientFD,TRSHandler->buffer,BUFFSIZE);
				*(TRSHandler->buffer+received) = '\0';
				fputs(TRSHandler->buffer,file);
				if(received < BUFFSIZE)
					break;
			}
			fclose(file);
			printf("received file %s\n     %ld Bytes\n",filename,size);
		}
		else
			printf("Error trying to download this file: %s\n",filename);
	}
	if(TRSHandler->clientFD)
		close(TRSHandler->clientFD);

}
/* -------------------------------------------------------------------------------------------- */

int TCPConnection(TCPHandler_p TRSHandler, const char *ip, const int port, const char *language){
	/* Estabilishes a TCP connection with the TRS server */
	struct hostent *addr;
	printf("%s %d\n",ip,port);

	if ((TRSHandler->clientFD = socket(AF_INET, SOCK_STREAM,0)) == -1)
		exitMsg("Error creating TCP socket");

	/* Configure settings of the TCP socket */
	TRSHandler->server.sin_family = AF_INET;
	TRSHandler->serverSize = sizeof(TRSHandler->server);
	if((addr = gethostbyname(ip) == NULL)) /* Check if TCS gave us an IP or hostname */
		TRSHandler->server.sin_addr.s_addr = inet_addr(ip);
	else
		TRSHandler->server.sin_addr.s_addr = ((struct in_addr *) (addr->h_addr_list[0]))->s_addr;
	
	TRSHandler->server.sin_port = htons(port);
	memset(TRSHandler->server.sin_zero, '\0', sizeof TRSHandler->server.sin_zero);

	strcpy(TRSHandler->language,language);
	if(connect(TRSHandler->clientFD, (struct sockaddr *) &TRSHandler->server, TRSHandler->serverSize))
		return 0;
	return 1;
}

int main(int argc, char **argv){
	
	char cmd[CMDSIZE];
	char option;
	unsigned short int defaultP=1,defaultA=1;
	UDPHandler_p TCSHandler;
	TCPHandler_p TRSHandler;
	char **languages = NULL; /* Hold the known languages */
	int langNumber = 0; /* Number of languages being hold */
	struct timeval tv; /* timeout */
	struct hostent *addr;

	/* Create TCP socket co communicate with the TRS's */
	TRSHandler = (TCPHandler_p) malloc(sizeof(struct TCPHandler));
	
	/* Create UDP socket to communicate with TCS */
	TCSHandler = (UDPHandler_p) malloc(sizeof(struct UDPHandler));
	TCSHandler->clientLen = sizeof(TCSHandler->client);

	if ( (TCSHandler->socket = socket(AF_INET, SOCK_DGRAM,0)) == -1)
		exitMsg("Error creating UDP socket");

	tv.tv_sec = 3;
	tv.tv_usec = 0;
	if (setsockopt(TCSHandler->socket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0)
    	exitMsg("Error in setsockopt");

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
			if((addr = gethostbyname(optarg)) == NULL)
				inet_aton(optarg , &TCSHandler->client.sin_addr);
			else
				TCSHandler->client.sin_addr.s_addr = ((struct in_addr *) (addr->h_addr_list[0]))->s_addr;
    			break;
    		case '?':
		        fprintf (stderr, "Unknown option `-%c'.\n", optopt);
    	}
    }
    if(defaultP){
    	/*printf("Using default port: %d\n",DEFAULTPORT);*/
    	TCSHandler->client.sin_port = htons(DEFAULTPORT);
    }
   	if(defaultA){
   		/*printf("Using default addr: %s\n",DEFAULTADDR);*/
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
			langNumber = list(TCSHandler,&languages);
		}
		else if(stringIn(cmd,REQCMD))
			request(TCSHandler,TRSHandler,cmd,languages,langNumber);
		else
			printf("Invalid command %s\n",cmd);

	}

	printf("Cleaning and exiting...\n");
	if(langNumber)
		cleanLanguagesList(languages,langNumber);
    cleanUDP(TCSHandler);
    free(TRSHandler);
	return 0;
}
