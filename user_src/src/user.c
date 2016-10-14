#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "user.h"
#include "util.h" /**/
#include <errno.h>


void cleanUDP(UDPHandler_p handler){
    /* Close TCS file descriptor and free the structure */
    close(handler->socket);
    free(handler);
}

void cleanLanguagesList(char **languages, int langNumber){
    int i;
    for(i = 0; i < langNumber; i++)
        free(languages[i]);
    free(languages);
}

void parseTCSOptions(UDPHandler_p TCSHandler, int argc, char **argv){
    char option;
    struct timeval tv; /* timeout for TCS */
    struct hostent *addr;
    unsigned short int defaultP = 1, defaultA = 1;

    TCSHandler->clientLen = sizeof(TCSHandler->client);
    if ( (TCSHandler->socket = socket(AF_INET, SOCK_DGRAM,0)) == -1)
        exitMsg("Error creating UDP socket");

    tv.tv_sec = 3; /* We wait 3 seconds max for an answer from TCS */
    tv.tv_usec = 0;
    if (setsockopt(TCSHandler->socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) /* Set timeout */
        exitMsg("Error in setsockopt");

    /* Initialize UDP socket */
    memset((char *) &TCSHandler->client, 0, sizeof(TCSHandler->client));

    /* Configure UDP socket and check args for optional address or port setting */
    while((option = getopt(argc, argv, "n:p:")) != -1){
        switch (option){
            case 'p': /* Set port */
                defaultP = 0;
                TCSHandler->client.sin_port = htons(atoi(optarg));
                break;
            case 'n': /* Set TCS hostname */
                if((addr = gethostbyname(optarg)) != NULL){ 
                    defaultA = 0;
                    TCSHandler->client.sin_addr.s_addr = ((struct in_addr *) (addr->h_addr_list[0]))->s_addr;
                }
                else
                    fprintf(stderr, "Invalid hostname\n");   /*If its not a valid hostname, keep the default (localhost) */
                break;
            case '?':
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        }
    }

    /* Set default server arguments if necessary */
    if(defaultP)
        TCSHandler->client.sin_port = htons(PORT);
    if(defaultA)
        inet_aton(DEFAULTADDR, &TCSHandler->client.sin_addr);

    TCSHandler->client.sin_family = AF_INET;
}

int safeSendUDP(UDPHandler_p TCSHandler, const char *toSend, unsigned int toSendLen){
    /* This function tries to send a message to TCS and receive an answer.
             If its not succeeded even after three tries, the program exits */
    int received, tries = 0;
    while(tries < 3){
        if (sendto(TCSHandler->socket, toSend, toSendLen, 0, (struct sockaddr *) &TCSHandler->client, TCSHandler->clientLen) == -1)
            exitMsg("Error sending message"); /* If there was an error sending the message, exit immediately */
        if ((received = recvfrom(TCSHandler->socket, TCSHandler->buffer, BUFFSIZE-1, 0, (struct sockaddr *) &TCSHandler->client, &TCSHandler->clientLen)) == -1){
            if(errno != ETIMEDOUT && errno != EAGAIN) /* If received timeout its fine, it will try to request data from TCS again */
                exitMsg("Error receiving messages");
        }
        else
            break;
        tries++;
    }
    if(tries < 3){
        *(TCSHandler->buffer+received) = '\0'; /* If it was succeeded, terminate string */
        return 1;
    }
    return 0;
}

/* ----------------------- Functions for list cmd -------------------------- */
int getLanguages(UDPHandler_p TCSHandler, char ***languages){
    /* Allocates space for language list and stores them in there */
    char *part;
    int langNumber;
    int i;

    /* ULR N language1 language2 ... languageN */
    part = strtok(TCSHandler->buffer, " "); /* ULR */ 
    if(part == NULL || strncmp(part, "ULR",3)){
        fprintf(stderr, "An error occured when receiving the languages list\n");
        return 0;
    }
    part = strtok(NULL, " "); /* N */
    langNumber = atoi(part);
    *languages = (char **) safeMalloc(sizeof(char *)*langNumber);
    for(i = 0; i < langNumber; i++)
        (*languages)[i] = (char *) safeMalloc(sizeof(char)*WORDSIZE);
    for(i = 0; i < langNumber - 1; i++){
        part = strtok(NULL, " "); /* get languagei from buffer */
        if (part == NULL) {
            fprintf(stderr, "TCS sent an invalid reply\n");
            cleanLanguagesList(*languages, langNumber);
            return 0;
        }
        strcpy((*languages)[i], part);
    }

    part = strtok(NULL, "\n"); /* last word ends in \n */
    if (part)
        strcpy((*languages)[i], part);
    else {
        fprintf(stderr, "TCS sent an invalid reply\n");
        cleanLanguagesList(*languages, langNumber);
        return 0;
    }

    return langNumber;
}

int list(UDPHandler_p TCSHandler, char ***languages){
    int langNumber;
    int i;

    /* Send User List Query */
    if(!safeSendUDP(TCSHandler, "ULQ\n",4)){
        printf("Couldn't send request to TCS server.\n");
        return 0;
    }
    langNumber = getLanguages(TCSHandler, languages);

    if(!langNumber)
        printf("No languages available\n");

    /* Print languages */
    for(i = 0; i < langNumber; i++)
        printf(" %d - %s\n", i+1, (*languages)[i]);
    return langNumber;
}

/* -------------------- Functions for request cmd ------------------------ */
int parseTCSUNR(UDPHandler_p TCSHandler, char **ip, unsigned int *port){
    char *part;

    /* Usual answer: UNR ip port */
    part = strtok(TCSHandler->buffer, " ");
    if(part == NULL || strcmp(part,"UNR")) /* UNR */
        return 0;
    
    *ip = strtok(NULL," ");
    if(*ip == NULL) /* ip */
        return 0;

    if(!strcmp(*ip,"EOF\n"))
        return 0;
    
    part = strtok(NULL, "\n"); /* port number should end in \n */
    if (!part)
	{
        fprintf(stderr, "TCS sent an invalid reply\n");
        return 0;
    }

    *port = atoi(part);
    return 1;
}

int sendUNQ(TCPHandler_p TRSHandler, UDPHandler_p TCSHandler, char **languages, int langName, char **ip, unsigned int *port){
    int total = 0, good = 0, received;
    while(!good && total < 3){ /* Tries 3 times */

        if(strcmp(TRSHandler->language, languages[langName])){ /* If user doesn't yet know where the specified TRS is... */
            /* Send UNQ + languageName */
            received = sprintf(TCSHandler->buffer, "%s %s\n", "UNQ", languages[langName]);

            if(!safeSendUDP(TCSHandler, TCSHandler->buffer, received)){ /* If it couldnt get an answer from TCS... */
                total++;
                continue;
            }
            
            if(!parseTCSUNR(TCSHandler, ip, port)){ /* If TCS didn't send a valid answer... */
                total++;
                continue;
            }

            good = TCPConnection(TRSHandler, *ip, *port, languages[langName]); /* Try to establish a TCP connection with TRS */
            total++;
        }
        else{
            good = TCPConnection(TRSHandler, inet_ntoa(TRSHandler->server.sin_addr), ntohs(TRSHandler->server.sin_port), languages[langName]);
            if(!good)
                memset(TRSHandler->language, 0, strlen(TRSHandler->language));
            else{
                *ip = inet_ntoa(TRSHandler->server.sin_addr);
            }
        }
    }

    if(total == 3)
        return 0;
    return 1;
}

char parseRequest(char *cmd, char *filename, char **words,int *langName, int *N){
    int i;
    char c;
    char *part;

    /* request N c args
    |         where N = languageNumber in the numbered list 
    |         where c = 't' (in that case args = words to be translated) 
    |             or c = 'f' (in that case args = filename) */

    part = strtok(cmd, " "); /* We already know this is "request" */
    part = strtok(NULL, " "); /* N */
    *langName = atoi(part)-1;

    if((part = strtok(NULL, " ")) == NULL) /* c */
        return '0';
    c = *part;
    if(c == 'f'){
        part = strtok(NULL, " "); /* filename */
        if(part == NULL)
            return '0';
        
        strcpy(filename, part);
        *(filename+strlen(filename)-1) = '\0';
    }
    else if(c == 't'){
        for(i = 0; i < 10; i++){
            words[i] = (char *) safeMalloc(WORDSIZE);
            if((part = strtok(NULL, " ")) == NULL) /* wordI to translate */
                break;
            strcpy(words[i],part);
        }
        *N = i;
    }
    else
        return '0';

    return c;
}

/* -------------------- Functions for handling text translations ----------------- */
void printWordsReceived(char *buffer){
    char *part;
    int N, i;
    part = strtok(buffer, " "); /* "TRR" */
    part = strtok(NULL, " ");   /* "t" */
    part = strtok(NULL, " ");   /* N = number of translated words */
    N = atoi(part);
    for(i = 0; i < N; i++){
        part = strtok(NULL, " ");
        if(part == NULL){
            printf("TRS said it would send %d words but it only sent %d\n", N, i);
            return;
        }
        printf(" %s", part);
    }
    puts("");
}

void handleTextTranslation(TCPHandler_p TRSHandler, char **words, char *ip,int N){
    int processedBytes,i;

    processedBytes = sprintf(TRSHandler->buffer, "TRQ t %d", N);
    for(i = 0; i < N; i++)
        processedBytes += sprintf(TRSHandler->buffer+processedBytes, " %s", words[i]);
    if(safe_write(TRSHandler->clientFD, TRSHandler->buffer, processedBytes) == -1){               /* Send  text translation request to TRS */
        puts("Couldn't send data to TRS. Maybe the connection was closed");
        return;
    }
 	
    /* Read TRS's answer */
    processedBytes = read_until_newline(TRSHandler->clientFD, TRSHandler->buffer, BUFFSIZE);
    if(processedBytes == -1){
        printf("Lost connection with TRS\n");
        return;
    }
    
    *(TRSHandler->buffer+processedBytes) = '\0';
    if(!strcmp(TRSHandler->buffer, "TRR NTA")){
        printf("No translation available\n");
        return;     
    }
    else if(!strcmp(TRSHandler->buffer, "TRR ERR")){
        printf("Too many words to translate\n");
        return;     
    }

    /* Free some resources */
    for(i = 0; i < N; i++)
        if(words[i])
            free(words[i]); 

    printf("  %s:", ip);
    printWordsReceived(TRSHandler->buffer);
}

/* -------------------- Functions for handling file translations ----------------- */
int sendFile(TCPHandler_p TRSHandler, char *filename){
    size_t size,bytes_read;
    int written=0;
    FILE *file;


    file = fopen(filename, "rb");
    if(!file){
        printf("File %s does not exist\n", filename);
        return 0;
    }

    if(fseek(file, 0L, SEEK_END) == -1){ /* ftell(file) will now tell us the file size */
        perror("Error reading file");
        return 0;
    }
    written = sprintf(TRSHandler->buffer, "TRQ f %s %ld ", filename, ftell(file));
    if(written < 0){
        printf("Error setting up data to send\n");
        return 0;
    }
    size = ftell(file);
    printf("     %ld Bytes to transmit\n", size);
     rewind(file);

    if(safe_write(TRSHandler->clientFD, TRSHandler->buffer, written) == -1){
        printf("Lost connection with TRS\n");
        return 0;
    }

    written = 0;
    while (written < size) {
        bytes_read = fread(TRSHandler->buffer, 1, BUFFSIZE, file);
        bytes_read = safe_write(TRSHandler->clientFD, TRSHandler->buffer, bytes_read);
        if(bytes_read == -1){
            printf("Lost connection with TRS\n");
            return 0;
        }
        written += bytes_read;
    }
    if(safe_write(TRSHandler->clientFD, "\n", 1) == -1){
        printf("Lost connection with TRS\n");
        return 0;
    }
    fclose(file);
    return 1;
}

int recvInitialData(TCPHandler_p TRSHandler, char *filename, unsigned long int *size){
    int i=0;

    if(read_until_space(TRSHandler->clientFD, TRSHandler->buffer, 4) == -1){/*TRR f filename size */
        printf("Lost connection with TRS\n");
        return 0;
    }

    /* to be read: f filename size */
    if(strcmp(TRSHandler->buffer, "TRR")){
    	printf("Bad answer from TRS\n");
    	return 0;
    }

    while(i != 2){
    	i += read(TRSHandler->clientFD, TRSHandler->buffer, 2);
    	if(!i){
    		printf("Lost connection with TRS\n");
    		return 0;
    	}
    }
    if(*TRSHandler->buffer != 'f'){
    	if(read_until_newline(TRSHandler->clientFD, TRSHandler->buffer+2, BUFFSIZE) == -1)
    		printf("Lost connection with TRS\n");
    	else if(!strcmp(TRSHandler->buffer, "NTA"))
        	printf("No translation available\n");           
        else if(!strcmp(TRSHandler->buffer, "ERR"))
        	printf("TRS ERR received. Please request translation again\n");
        else
        	printf("An error occured while receiving file translation. Please repeat the request %s\n",TRSHandler->buffer);
        return 0;
    }

    if(read_until_space(TRSHandler->clientFD, filename, 100) == -1){
    	printf("Lost connection with TRS\n");
    	return 0;
    }

    i = 0;
    while(1){ /* Receive file size */
        while(!read(TRSHandler->clientFD, TRSHandler->buffer+i, 1));
        if(*(TRSHandler->buffer+i) == ' ')
            break;
        i += 1;
    }
    *(TRSHandler->buffer+i) = '\0';
    *size = atol(TRSHandler->buffer); 

    return 1;
}

int recvFile(TCPHandler_p TRSHandler,char *filename, unsigned long int size){
	unsigned long int total = 0;
	int processedBytes;
	FILE *file = fopen(filename, "wb");

	if(file == NULL)
		return 0;
	while(total < size){
        processedBytes = read(TRSHandler->clientFD, TRSHandler->buffer, MIN(BUFFSIZE, size - total));
        if(!processedBytes){
            perror("Error in read");
            return 0;
        }
        total += processedBytes;
        fwrite(TRSHandler->buffer, 1, processedBytes, file);
    }

    /*TRS should have sent a newline after the file data */
    processedBytes = 0;
    fclose(file);
    while(!processedBytes){
    	processedBytes = read(TRSHandler->clientFD, TRSHandler->buffer, 1);
		if(processedBytes == -1){
			perror("TCS didnt send newline. File might be corrupted");
			return 0;
		}
    }
    return 1;
}

void handleFileTranslation(TCPHandler_p TRSHandler, char *filename){
    unsigned long int size;

    if(!sendFile(TRSHandler, filename))
        return;

    if(!recvInitialData(TRSHandler, filename, &size)) /* TRR f filename size */
        return;

    
    if(!recvFile(TRSHandler, filename, size)){
        printf("Error trying to download this file: %s\n", filename);
        return;
    }

    printf("received file %s\n     %ld Bytes\n", filename, size);
        
}

int request(UDPHandler_p TCSHandler,TCPHandler_p TRSHandler, char *cmd, char **languages, int langNumber){
    char *words[10];    
    int i = 0;
    int langName, N=0;
    char filename[100];
    char c;
    char *ip = NULL;
    unsigned int port;

    c = parseRequest(cmd, filename, words, &langName, &N);
    if(c == '0'){
        puts("Not enough arguments or invalid request");
        return langNumber;
    }
    else if(langName >= langNumber){
        puts("You tried to translate from an invalid language. You can type 'list' to get the list of languages.");
        if(c == 't'){
            for (i = 0; i < N; i++)
                if(words[i])
                    free(words[i]);
        }
        return langNumber; 
    }
    
    /* Check if the user wants to send an invalid file */
    if(c == 'f'){
        FILE *f = fopen(filename,"rb");
        if(f == NULL){
            printf("Invalid file\n");
            return langNumber;
        }

        fclose(f);
    }

    if(!sendUNQ(TRSHandler,TCSHandler,languages,langName,&ip,&port)){ /* Asks TCS for TRS location */
        printf("An error occured while requesting TRS server address. Sending list request again...\n");
        if(langNumber)
            cleanLanguagesList(languages, langNumber);
        printf("\n\n\n");
        langNumber = list(TCSHandler, &languages);
        return langNumber;
    }

    if(c == 't')
        handleTextTranslation(TRSHandler, words, ip, N);
    else
        handleFileTranslation(TRSHandler, filename);
        
    if(TRSHandler->clientFD)
        close(TRSHandler->clientFD);
    return langNumber;

}
/* -------------------------------------------------------------------------------------------- */

int TCPConnection(TCPHandler_p TRSHandler, const char *ip, const int port, const char *language){
    /* Establishes a TCP connection with the TRS server */
    struct timeval tv; /* timeout */
    struct hostent *addr;
    printf(" %s %d\n", ip, port);

    if ((TRSHandler->clientFD = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        exitMsg("Error creating TCP socket");

    /* Configure settings of the TCP socket */
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    TRSHandler->server.sin_family = AF_INET;
    TRSHandler->serverSize = sizeof(TRSHandler->server);
    if((addr = gethostbyname(ip)) == NULL) /* Check if TCS gave us an IP or hostname */
        TRSHandler->server.sin_addr.s_addr = inet_addr(ip);
    else
        TRSHandler->server.sin_addr.s_addr = ((struct in_addr *) (addr->h_addr_list[0]))->s_addr;
    
    TRSHandler->server.sin_port = htons(port);
    memset(TRSHandler->server.sin_zero, '\0', sizeof TRSHandler->server.sin_zero);

    strcpy(TRSHandler->language,language);
    if(connect(TRSHandler->clientFD, (struct sockaddr *) &TRSHandler->server, TRSHandler->serverSize)){
        printf("Could not connect to TRS\n");
        return 0;
    }
	if (setsockopt(TRSHandler->clientFD, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0)
		exitMsg("Error in setsockopt");
	if (setsockopt(TRSHandler->clientFD, SOL_SOCKET, SO_SNDTIMEO,&tv,sizeof(tv)) < 0)
		exitMsg("Error in setsockopt");
    return 1;
}

int main(int argc, char **argv){
    
    char cmd[CMDSIZE];
    UDPHandler_p TCSHandler;
    TCPHandler_p TRSHandler;
    char **languages = NULL; /* Hold the known languages */
    int langNumber = 0; /* Number of languages being hold */

    /* Create TCP socket to communicate with the TRS's */
    TRSHandler = (TCPHandler_p) safeMalloc(sizeof(struct TCPHandler));
    
    /* Create UDP socket to communicate with TCS */
    TCSHandler = (UDPHandler_p) safeMalloc(sizeof(struct UDPHandler));

    parseTCSOptions(TCSHandler, argc, argv);

    /* Repeatedly read command, execute command and print results */
    while(1){
        if(fgets(cmd, CMDSIZE, stdin) == NULL)
            break;
        if(!strcmp(cmd, EXITCMD))
            break;
        else if(!strcmp(cmd, LISTCMD)){ /* if it's a list request */
            if(langNumber)
                cleanLanguagesList(languages, langNumber);
            langNumber = list(TCSHandler, &languages);
        }
        else if(!strncmp(cmd, REQCMD, strlen(REQCMD))) /* If it's a translation request */
            langNumber = request(TCSHandler, TRSHandler, cmd, languages, langNumber);
        else
            printf("Invalid command %s\n", cmd);

    }

    printf("Cleaning and exiting...\n");
    if(langNumber)
        cleanLanguagesList(languages, langNumber);
    cleanUDP(TCSHandler);
    free(TRSHandler);
    return 0;
}
