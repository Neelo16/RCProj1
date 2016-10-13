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
    close(handler->socket);
    free(handler);
}

void cleanLanguagesList(char **languages,int langNumber){
    int i;
    for(i = 0; i < langNumber; i++)
        free(languages[i]);
    free(languages);
}

void parseTCSOptions(UDPHandler_p TCSHandler,int argc, char **argv){
    char option;
    struct timeval tv; /* timeout */
    struct hostent *addr;
    unsigned short int defaultP=1,defaultA=1;

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
                break;
            case 'n':
                if((addr = gethostbyname(optarg)) != NULL){
                    defaultA = 0;
                    TCSHandler->client.sin_addr.s_addr = ((struct in_addr *) (addr->h_addr_list[0]))->s_addr;
                }
                break;
            case '?':
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        }
    }
    if(defaultP)
        TCSHandler->client.sin_port = htons(PORT);
    if(defaultA)
        inet_aton(DEFAULTADDR , &TCSHandler->client.sin_addr);

    TCSHandler->client.sin_family = AF_INET;
}

int safeSendUDP(UDPHandler_p TCSHandler, const char *toSend, unsigned int toSendLen){
    int received,tries = 0;
    while(tries < 3){
        if (sendto(TCSHandler->socket, toSend, toSendLen , 0 , (struct sockaddr *) &TCSHandler->client, TCSHandler->clientLen) == -1)
            exitMsg("Error sending message");
        if ((received = recvfrom(TCSHandler->socket, TCSHandler->buffer, BUFFSIZE-1, 0, (struct sockaddr *) &TCSHandler->client, &TCSHandler->clientLen)) == -1){
            if(errno != ETIMEDOUT && errno != EAGAIN)
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
    if(part == NULL || strncmp(part,"ULR",3)){
        printf("An error occured when receiving the languages list\n");
        return 0;
    }
    part = strtok(NULL," ");
    langNumber = atoi(part);
    *languages = (char **) safeMalloc(sizeof(char *)*langNumber);
    for(i = 0; i < langNumber; i++)
        (*languages)[i] = (char *) safeMalloc(sizeof(char)*WORDSIZE);
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
    if(!safeSendUDP(TCSHandler,SENDULQ,SENDULQSIZE)){
        printf("Couldn't send request to TCS server.\n");
        return 0;
    }
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
    *ip = strtok(NULL," ");
    if(*ip == NULL){
        printf("Didnt receive enough data from TCS2: %s\n",TCSHandler->buffer);
        return 0;
    }
    part = strtok(NULL, " ");
    if (part == NULL){
        printf("Didnt receive enough data from TCS3: %s\n",TCSHandler->buffer);
        return 0;
    }   
    *port = atoi(part);
    return 1;
}

int checkReceive(TCPHandler_p TRSHandler, int toReceive){
    int total = 0,received = 0;
    while(total != toReceive){
        received= read(TRSHandler->clientFD,TRSHandler->buffer+total,toReceive);
        if(received == -1){
            printf("Couldnt receive enough data\n");
            return 0;
        }
        total += received;
    }
    return 1;
}

int sendUNQ(TCPHandler_p TRSHandler, UDPHandler_p TCSHandler, char **languages, int langName, char **ip, unsigned int *port){
    int total = 0,good = 0,received;
    while(!good && total < 3){ /* Tries 3 times */

        if(strcmp(TRSHandler->language,languages[langName])){

            /* Send UNQ + languageName */
            received = sprintf(TCSHandler->buffer,"%s %s\n","UNQ",languages[langName]);

            if(!safeSendUDP(TCSHandler,TCSHandler->buffer,received))
                return 0;
            if(!parseTCSUNR(TCSHandler,ip, port))
                return 0;
            good = TCPConnection(TRSHandler, *ip, *port, languages[langName]);
        }
        else{
            good = !connect(TRSHandler->clientFD, (struct sockaddr *) &TRSHandler->server, TRSHandler->serverSize);
            if(!good)
                memset(TRSHandler->language,0,strlen(TRSHandler->language));
        }
        total++;
    }

    if(total == 3){
        printf("Could not connect to TRS server\n");
        return 0;
    }
    return 1;
}

char parseRequest(char *cmd, char *filename, char **words,int *langName, int *N){
    int i;
    char c;
    char *part;

    part = strtok(cmd," ");
    part = strtok(NULL," ");
    *langName = atoi(part)-1;

    if((part = strtok(NULL," ")) == NULL)
        return '0';
    c = *part;
    if(c == 'f'){
        part = strtok(NULL, " ");
        if(part == NULL)
            return '0';
        
        strcpy(filename,part);
        *(filename+strlen(filename)-1) = '\0';
    }
    else if(c == 't'){
        for(i = 0; i < 10; i++){
            words[i] = (char *) safeMalloc(sizeof(char)*WORDSIZE);
            if((part = strtok(NULL," ")) == NULL)
                break;
            strcpy(words[i],part);
        }
        *N = i;
    }
    else
        return '0';

    return c;
}

void printWordsReceived(char *buffer){
    char *part;
    int N,i;
    part = strtok(buffer," ");
    part = strtok(NULL," ");
    part = strtok(NULL," ");
    N = atoi(part);
    for(i = 0; i < N; i++){
        part = strtok(NULL," ");
        if(part == NULL){
            printf("TRS said he would send %d words but he only sent %d\n",N,i);
            return;
        }
        printf(" %s",part);
    }
    puts("");
}

void handleTextTranslation(TCPHandler_p TRSHandler, char **words, char *ip,int N){
    int processedBytes,i,total=0;

    processedBytes = sprintf(TRSHandler->buffer, "TRQ t %d",N);
    for(i = 0; i < N; i++)
        processedBytes += sprintf(TRSHandler->buffer+processedBytes," %s",words[i]);
    write(TRSHandler->clientFD,TRSHandler->buffer,processedBytes);

    /* *(TRSHandler->buffer) = '\0'; */
    while(processedBytes != -1){
        processedBytes = read(TRSHandler->clientFD,TRSHandler->buffer+total,BUFFSIZE-total-1);
        total += processedBytes;
        if(processedBytes && *(TRSHandler->buffer+total-1) == '\n')
            break;
    }
    if(processedBytes == -1){
        perror("Couldnt receive data from TRS");
        return;
    }

    total = 0;
    *(TRSHandler->buffer+processedBytes) = '\0';
    if(!strcmp(TRSHandler->buffer,"TRR NTA\n")){
        printf("No translation available\n");
        return;     
    }
    else if(!strcmp(TRSHandler->buffer,"TRR ERR\n")){
        printf("Too many words to translate\n");
        return;     
    }

    /* Free some resources */
    for(i = 0; i < N; i++)
        if(words[i])
            free(words[i]); 

    printf("  %s:",ip);
    printWordsReceived(TRSHandler->buffer);
}

int sendFile(TCPHandler_p TRSHandler, char *filename){
    long unsigned int size;
    int written=0;
    FILE *file;


    file = fopen(filename,"rb");
    if(!file){
        printf("File %s does not exist\n",filename);
        return 0;
    }

    if(fseek(file, 0L, SEEK_END) == -1){
        perror("Error reading file");
        return 0;
    }
    written = sprintf(TRSHandler->buffer, "TRQ f %s %ld ",filename,ftell(file));
    if(written < 0){
        printf("Error setting up data to send\n");
        return 0;
    }
    size = ftell(file);
    printf("     %ld Bytes to transmit\n",size);
    rewind(file);

    write(TRSHandler->clientFD,TRSHandler->buffer,written);

    while (written < size) {
        int read = fread(TRSHandler->buffer, 1, BUFFSIZE, file);
        written += write(TRSHandler->clientFD, TRSHandler->buffer, read);
    }
    write(TRSHandler->clientFD,"\n",1);
    fclose(file);
    return 1;
}

int recvInitialData(TCPHandler_p TRSHandler, char *filename, unsigned long int *size){
    int i=0;
    char c;

    if(!checkReceive(TRSHandler,6))/*TRR f */
        return 0;
    if(!strcmp(TRSHandler->buffer,"TRR NTA\n")){
        printf("No translation available\n");           
        return 0;
    }
    else if(!strcmp(TRSHandler->buffer,"TRR ERR\n")){
        printf("TRS probably didnt receive the correct data\n");
        return 0;     
    }
    while(1){ /* Receive filename */
        read(TRSHandler->clientFD,&c,1);
        if(c == ' ')
            break;
        *(filename+i) = c;
        i++;
    }
    *(filename+i) = '\0';

    i = 0;
    while(1){ /* Receive file size */
        while(!read(TRSHandler->clientFD,TRSHandler->buffer+i,1));
        if(*(TRSHandler->buffer+i) == ' ')
            break;
        i += 1;
    }
    *(TRSHandler->buffer+i) = '\0';
    *size = atol(TRSHandler->buffer); 

    return 1;
}

void handleFileTranslation(TCPHandler_p TRSHandler, char *filename){
    unsigned long int size;
    int processedBytes;
    int total = 0;
    FILE *file;


    if(!sendFile(TRSHandler,filename))
        return;

    if(!recvInitialData(TRSHandler,filename,&size))
        return;
    
    file = fopen(filename, "wb");
    if(file == NULL){
        printf("Error trying to download this file: %s\n",filename);
        return;
    }

    while(1){
        processedBytes = read(TRSHandler->clientFD,TRSHandler->buffer,MIN(BUFFSIZE, size - total));
        if(!processedBytes){
            perror("Erro");
            return;
        }
        total += processedBytes;
        fwrite(TRSHandler->buffer,1,processedBytes,file);
        if(total >= size)
            break;
    }
    printf("received file %s\n     %ld Bytes\n",filename,size);
    fclose(file);
        
}

void request(UDPHandler_p TCSHandler,TCPHandler_p TRSHandler, char *cmd, char **languages, int langNumber){
    char *words[10];    
    int i = 0;
    int langName,N=0;
    char filename[100];
    char c;
    char *ip = NULL;
    unsigned int port;

    c = parseRequest(cmd,filename,words,&langName,&N);
    if(c == '0'){
        printf("Not enough arguments or invalid request\n");
        return;
    }
    else if(langName >= langNumber){
        printf("You tried to translate from an invalid language. You can type 'list' to get the list of languages.\n");
        if(c == 't'){
            for (i = 0; i < N; i++)
                if(words[i])
                    free(words[i]);
        }
        return; 
    }
    
    if(!sendUNQ(TRSHandler,TCSHandler,languages,langName,&ip,&port))
        return;

    if(c == 't')
        handleTextTranslation(TRSHandler,words,ip,N);
    else
        handleFileTranslation(TRSHandler,filename);
        
    if(TRSHandler->clientFD)
        close(TRSHandler->clientFD);

}
/* -------------------------------------------------------------------------------------------- */

int TCPConnection(TCPHandler_p TRSHandler, const char *ip, const int port, const char *language){
    /* Estabilishes a TCP connection with the TRS server */
    struct timeval tv; /* timeout */
    struct hostent *addr;
    printf(" %s %d\n",ip,port);

    if ((TRSHandler->clientFD = socket(AF_INET, SOCK_STREAM,0)) == -1)
        exitMsg("Error creating TCP socket");

    /* Configure settings of the TCP socket */
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if (setsockopt(TRSHandler->clientFD, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0)
        exitMsg("Error in setsockopt");

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
        printf("Couldn't connect to TRS\n");
        return 0;
    }
    return 1;
}

int main(int argc, char **argv){
    
    char cmd[CMDSIZE];
    UDPHandler_p TCSHandler;
    TCPHandler_p TRSHandler;
    char **languages = NULL; /* Hold the known languages */
    int langNumber = 0; /* Number of languages being hold */

    /* Create TCP socket co communicate with the TRS's */
    TRSHandler = (TCPHandler_p) safeMalloc(sizeof(struct TCPHandler));
    
    /* Create UDP socket to communicate with TCS */
    TCSHandler = (UDPHandler_p) safeMalloc(sizeof(struct UDPHandler));

    parseTCSOptions(TCSHandler,argc,argv);

    /* Repeatedly read command, execute command and print results */
    while(1){
        if(fgets(cmd,CMDSIZE,stdin) == NULL)
            break;
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
