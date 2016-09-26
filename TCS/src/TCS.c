#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "TCS.h"
#include "queue.h"

#define PORT 58000
#define MAX 1024

char* getBufferLanguage(char buffer[])
{
    char* language;

    printf("Buffer: %s\n",buffer);
    language = strtok(buffer, " ");/* ULQ */
    language = strtok(NULL, " ");/* language */

    /* The request is not formulated correcly if the user doesnt 
    add any language or more than one */
    if(language == NULL || strtok(NULL, " ") != NULL)
        strcpy(language,"UNR ERR");

    return language;
}

char* getTRSInfo(trs_list list, char* language)
{
    trs_item trs = (trs_item) malloc(sizeof(struct trsItem));
    char* repply = (char*) malloc(sizeof(char)*128);

    int repplyLen = 0;

    trs = findTRS(list, language);
    printf("%s\n",language);
    /* the language doesnt exist in the server_list */
    if( trs == NULL)
    {
        strcpy(repply, "UNR EOF\n");
        return repply;
    }

    repplyLen = sprintf(repply, "UNR ");
    repplyLen = sprintf(repply+repplyLen,"%s %d\n",getIp(trs),getPort(trs));
    
    return repply;
    
}

int main(int argc, const char **argv)  {

    int user=0; /* trs; sockets */
    /*struct hostent *hostptr;*/
    struct sockaddr_in serveraddr, clientaddr;
    unsigned int addrlen;
    int port;
    char buffer[MAX];
	char* repply = (char *) malloc(sizeof(char)*MAX);
    char* language = (char *) malloc(sizeof(char)*MAX);
    int received = 0;

	/* Create Server List */
	trs_list server_list = createList(); 
    addTRSItem(server_list, createTRS("Portugues", "13245", 59000));
    addTRSItem(server_list, createTRS("Coreano", "13242", 59020));

    /*  Port assignment */
    if( argc > 1)
    {
        port = atoi(argv[1]);
    }else /* in case -p TCSport is omitted */
    {
        port = PORT;
    }


    /* USER */
    
    user = socket(AF_INET, SOCK_DGRAM,0);

    memset((void*)&serveraddr, (int)'\0',sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);
    
    bind(user, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    addrlen = sizeof(clientaddr);

    while(1)
    {
        received = recvfrom(user, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientaddr, &addrlen);
        *(buffer + received) = '\0';
        printf("%s\n",buffer);
        if(!strncmp(buffer, "ULQ", 3))
        {
            if(strlen(buffer) > 4)
            {
                strcpy(repply,"URR\n\0");
                sendto(user, repply, 4, 0, (struct sockaddr*) &clientaddr, addrlen);
            }
            else if(server_list->size == 0)
            {
                strcpy(repply,"ULR EOF\n");
                sendto(user, repply, 8, 0, (struct sockaddr*) &clientaddr, addrlen);
            }
            else
            {
                repply = listLanguages(server_list);
                sendto(user, repply, strlen(repply), 0, (struct sockaddr*) &clientaddr, addrlen);
            }
        }
        else if(!strncmp(buffer,"UNQ",3))
        {
            printf("Oh\n");
            /* gets language requested from the user */
            language = getBufferLanguage(buffer); 
            
            /* looks for the language in trs list and returs its info */
            repply = getTRSInfo( server_list, language);
            sendto(user, repply, strlen(repply), 0, (struct sockaddr*) &clientaddr, addrlen);
            
        }

        /* TRS 

        trs = socket(AF_INET, SOCK_DGRAM, 0);

        memset((void*)&serveraddr, (int)'\0', sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serveraddr.sin_port = htons((u_short)port);
        
        bind(trs, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
        addrlen = sizeof(clientaddr);
        */  
    }
    

    return 0;

}     
