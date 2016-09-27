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
    int i, aux = 0;
    char* language;
    
    language = strtok(buffer, " ");

    for(i = 0 ; buffer[i] != '\0'; i++)
    {
        if(buffer[i] == ' ')
            aux++;
    }
    /* The format is not formulated correcly if the user
     * doesnt add any language or adds more than one.*/
    if((aux == 0 || aux > 1)&& language == "UNQ")
        return "UNR ERR\n";
    else if( aux == 3 && language == "SRG")
    {
        return "SRR ERR\n"; 
    }
    else if( aux == 3 && language == "SUN")
    {
        return "SUR ERR\n";
    }
    else
    {
        language = strtok(NULL, " "); /*language*/
        return language;
    }
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
    repplyLen = sprintf(repply + repplyLen,"%s %d\n",getIp(trs),getPort(trs));
    
    return repply;
    
}

char* checkTRS(trs_list list, char buffer[])
{
    char* status;
    char* language;
    char* ip;
    char* port;
    int i, aux = 0;
    trs_item trs = (trs_item) malloc(sizeof(struct trsItem));

    language = getBufferLanguage(buffer);
    
    /*error case*/
    if(!strcmp(language, "SRR ERR"))
        return language;
    else
    {
        /* checks if the trs is already in the server_list*/
        trs = findTRS(list, language);
        
        /* if not it adds to the list and sends status OK*/
        if(trs == NULL)
        {
            ip = strtok(NULL, " ");
            port = strtok(NULL, " ");
           
            trs = createTRS(language, ip, atoi(port));
           
            addTRSItem(list, trs);

            strcpy(status, "status = OK\n");
            
            return status;
        }
        /* otherwise sends status not ok*/
        else
        {
            destroyTRS(trs);
            strcpy(status, "status = NOK\n");
            return status;
        }
    }
}

/*FIXME */
char* stopTranslating(trs_list list, char buffer[])
{
    char* language;
    char* status;
    trs_item trs = (trs_item)malloc(sizeof(struct trsItem));

    language = getBufferLanguage(buffer);
    
    if(!strcmp(language, "SUR ERR"))
    {
        destroyTRS(trs);
        strcpy(status, "status = NOK");
        return status;
    }
    else
    {
        trs = findTRS(list, language);
        strcpy(status, "status = OK");

        if(trs == NULL)
        {
            destroyTRS(trs);
            return status;
        }
        else
        {
            removeTRS(list, language);
            return status;
        }
    }

}

int main(int argc, const char **argv)  {

    int user = 0; /* socket */
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
    
    /*addTRSItem(server_list, createTRS("Portugues", "13245", 59000));
    addTRSItem(server_list, createTRS("Coreano", "13242", 59020));
    */

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
            /* gets language requested from the user */
            language = getBufferLanguage(buffer); 
            
            /* looks for the language in trs list and returs its info */
            repply = getTRSInfo( server_list, language);
            sendto(user, repply, strlen(repply), 0, (struct sockaddr*) &clientaddr, addrlen);
            
        }
        else if(!strncmp( buffer, "SRG", 3))
        {
            /*confirms if the trs is in server_list*/
            repply = checkTRS(server_list, buffer);

            /*sends its status to TRS*/
            sendto(user, repply, strlen(repply), 0, (struct sockaddr*) &clientaddr, addrlen);   
        }
        else if(!strncmp( buffer, "SUN", 3))
        {
            /* the TCS looks for the trs in the server_list and deletes it*/
            /*repply = stopTranslating(server_list, buffer);
            
            sendto(user, repply, strlen(repply), 0, (struct sockaddr*) &clientaddr, addrlen);    
            */
        }
    }
    

    return 0;

}     
