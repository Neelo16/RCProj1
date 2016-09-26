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
#define MAX 100

char* getBufferLanguage(char buffer[])
{
    char* language = malloc(sizeof(char)*20);

    language = strtok(buffer, " ");//ULQ
    language = strtok(NULL, " ");//language
    
    //The request is not formulated correcly //FIXME
    if( !strcmp(language, ""))
    {
        return "UNR ERR";
    }

    return language;
}

char* getTRSInfo(trs_list list, char* language)
{
    trs_item trs = malloc(sizeof(struct trsItem));
    char* repply, *aux;

    trs = findTRS(list, language);
    
    //the language doesnt exist in the server_list
    if( trs == NULL)
    {
        return "UNR EOF";
    }
    
    repply = strcat(repply, "UNR ");

    repply = strcat(repply, getHostname(trs));
    
    sprintf(aux, "%d", getPort(trs));
    repply = strcat( repply, aux);
    
    return repply;
    
}

int main(int argc, const char *argv[])  {

    int user, trs; //sockets
    struct hostent *hostptr;
    struct sockaddr_in serveraddr, clientaddr;
    int addrlen;
    int port;
    char buffer[MAX];
	char* repply = malloc(sizeof(char)*MAX);
    char* language = malloc(sizeof(char)*MAX);


	//Create Server List
	trs_list server_list = createList();

    // Port assignment 
    if( sizeof(argv) > 1)
    {
        port = atoi(argv[2]);
    }else //in case -p TCSport is omitted
    {
        port = PORT;
    }

    /* USER */
    
    user = socket(AF_INET, SOCK_DGRAM,0);

    memset((void*)&serveraddr, (int)'\0',sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((u_short)port);
    
    bind(user, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    addrlen = sizeof(clientaddr);
    
 
    recvfrom(user, buffer,sizeof(buffer), 0, (struct sockaddr*)&clientaddr, &addrlen);

    if(!strncmp(buffer, "ULQ", 3))
    {
        if(server_list->size == 0)
        {
            repply = "ULR EOF\n";
            sendto(user, repply, sizeof(buffer), 0, NULL, 0);
        }
       //URR ??
        else
        {
            repply = listLanguages(server_list);
            sendto(user, repply, sizeof(repply), 0, NULL, 0);
        }
    }
    else if(!strncmp(buffer,"UNQ",3))
    {
        //gets language requested from the user
        language = getBufferLanguage(buffer); 
        
        //looks for the language in trs list and returs its info
        repply = getTRSInfo( server_list, language);
        sendto(user, repply, sizeof(repply), 0, NULL, 0);
        
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
