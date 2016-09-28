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


void getBufferLanguage(char buffer[], char language2[])
{
    int i, aux = 0;

    char *language;

    for(i = 0 ; buffer[i] != '\0'; i++)
    {
        if(buffer[i] == ' ')
            aux++;
    }

    language = strtok(buffer, " ");

    /* The format is not formulated correcly if the user
     * doesnt add any language or adds more than one.*/
    if((aux == 0 || aux > 1)&& !strcmp(language, "UNQ"))
        strcpy(language2, "UNR ERR\n");

    else if( aux != 3 && !strcmp(language,"SRG"))
        strcpy(language2,"SRR ERR\n"); 

    else if( aux != 3 && !strcmp(language,"SUN"))
        strcpy(language2,"SUR ERR\n");

    else
    {
        language = strtok(NULL, " \n"); /*language*/
        strcpy(language2,language);

    }
}

void getTRSInfo(trs_list list, char* language, char repply[])
{
    trs_item trs = (trs_item) malloc(sizeof(struct trsItem));

    int repplyLen = 0;

    trs = findTRS(list, language);
    printf("%s\n",language);

    /* the language doesnt exist in the server_list */
    if( trs == NULL)
    {
    	destroyTRS(trs);
        strcpy(repply, "UNR EOF\n");
    }
    else
    {
    	repplyLen = sprintf(repply, "UNR ");
    	repplyLen = sprintf(repply + repplyLen,"%s %s %d\n",language, getIp(trs),getPort(trs));

    	destroyTRS(trs);
    }

    
}

void checkTRS(trs_list list, char buffer[], char repply[])
{
    char language[MAX];
    char* ip;
    char* port;
    
    trs_item trs = (trs_item) malloc(sizeof(struct trsItem));

    getBufferLanguage(buffer, language);
    
    /*error case*/
    if(!strcmp(language, "SRR ERR\n"))
    {
        strcpy(repply, language);

        destroyTRS(trs);
    }
    else
    {
        /* checks if the trs is already in the server_list*/
        trs = findTRS(list, language);

        /* if not it adds to the list and sends status OK*/
        if(trs == NULL)
        {
            ip = strtok(NULL, " \n");
            port = strtok(NULL, " \n");
           
            trs = createTRS(language, ip, atoi(port));
           
            addTRSItem(list, trs);

            strcpy(repply, "SRR OK\n");            
        }
        /* otherwise sends status not ok*/
        else
            strcpy(repply, "SRR NOK\n");

        destroyTRS(trs);
    }
}


void stopTranslating(trs_list list, char buffer[], char repply[])
{
    char language[MAX];
    trs_item trs = (trs_item)malloc(sizeof(struct trsItem));

    getBufferLanguage(buffer, language);
    
    if(!strcmp(language, "SUR ERR\n"))
    {
        strcpy(repply, "SUR NOK\n");

        destroyTRS(trs);
    }
    else
    {
        trs = findTRS(list, language);

        strcpy(repply, "SUR OK\n");

        if(trs == NULL)
            destroyTRS(trs);
        else
            removeTRS(list, language);
    }
}

void finishProgram(int* user, trs_list list)
{
	if(close(*user) == -1)
		perror("An error occurred on close");
	destroyList(list);
}

int main(int argc, const char **argv)  {

    int user = 0; /* socket */
    struct sockaddr_in serveraddr, clientaddr;
    unsigned int addrlen;
    int port, error;
    char buffer[MAX];
	char repply[MAX];
    char language[MAX];
    int received = 0;
    int retval;
    trs_list server_list;
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);


	/* Create Server List */
	server_list = createList(); 
    
    addTRSItem(server_list, createTRS("Portugues", "13245", 59000));
    addTRSItem(server_list, createTRS("Coreano", "13242", 59020));
    
    /*  Port assignment */
    if( argc > 2 && !strcmp(argv[1], "-p"))
        port = atoi(argv[1]);
    else /* in case -p TCSport is omitted */
        port = PORT;


    /* USER */
    
    user = socket(AF_INET, SOCK_DGRAM,0);

    if(user == -1){
    	perror("An error occurrer on socket");
    	return EXIT_FAILURE;
    }

    memset((void*)&serveraddr, (int)'\0',sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);
    
    error = bind(user, (struct sockaddr*)&serveraddr, sizeof(serveraddr));

    if(error == -1){
    	perror("An error occurrer an error on bind");
    	return EXIT_FAILURE;
    }

    addrlen = sizeof(clientaddr);

    while(1)
    {
    	FD_ZERO(&rfds);
        FD_SET(0, &rfds);
    	FD_SET(user, &rfds);

    	retval = select(user+1, &rfds, NULL, NULL, NULL);

    	if(retval == -1) {
    		perror("Occurrer an error on select");
    		finishProgram(&user, server_list);
    		return EXIT_FAILURE;
    	}
    	else if( FD_ISSET(user, &rfds))
    	{
    		
	        received = recvfrom(user, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientaddr, &addrlen);
	        
	        if(received == -1)
	        {
	        	perror("An error occurred on recvfrom");
	            finishProgram(&user, server_list);
	            return EXIT_FAILURE;
	        }

	        *(buffer + received) = '\0';
	        printf("%s",buffer);

	        if(!strncmp(buffer, "ULQ", 3))
	        {
	            if(strlen(buffer) > 4)
	            {
	                strcpy(repply,"URR\n");
	                error = sendto(user, repply, 4, 0, (struct sockaddr*) &clientaddr, addrlen);

	                if(error == -1)
	                {
	                	perror("An error occurred on sendto");
	                	finishProgram(&user, server_list);
	                	return EXIT_FAILURE;
	                }
	            }
	            else if( sizeList(server_list) == 0)
	            {
	                strcpy(repply,"ULR EOF\n");
	                error = sendto(user, repply, 8, 0, (struct sockaddr*) &clientaddr, addrlen);

	                if(error == -1)
	                {
	                	perror("An error occurred on sendto");
	                	finishProgram(&user, server_list);
	                	return EXIT_FAILURE;
	                }
	            }
	            else
	            {
	                listLanguages(server_list, repply);
	                error = sendto(user, repply, strlen(repply), 0, (struct sockaddr*) &clientaddr, addrlen);

	                if(error == -1)
	                {
	                	perror("An error occurred on sendto");
	                	finishProgram(&user, server_list);
	                	return EXIT_FAILURE;
	                }
	            }
	        }
	        else if(!strncmp(buffer, "UNQ", 3))
	        {
	            /* gets language requested from the user */
	            getBufferLanguage(buffer, language); 
	            
	            /* looks for the language in trs list and returs its info */
	            getTRSInfo( server_list, language, repply);
	            error = sendto(user, repply, strlen(repply), 0, (struct sockaddr*) &clientaddr, addrlen);

	            if(error == -1)
	            {
	              	perror("An error occurred on sendto");
	              	finishProgram(&user, server_list);
	                return EXIT_FAILURE;
	            }
	            
	        }
	        else if(!strncmp( buffer, "SRG", 3))
	        {
	            /*confirms if the trs is in server_list*/
	            checkTRS(server_list, buffer, repply);

	            /*sends its status to TRS*/
	            error = sendto(user, repply, strlen(repply), 0, (struct sockaddr*) &clientaddr, addrlen);

	            if(error == -1)
	            {
	              	perror("An error occurred on sendto");
	              	finishProgram(&user, server_list);
	                return EXIT_FAILURE;
	            }
	        }
	        else if(!strncmp( buffer, "SUN", 3))
	        {
	            /* if the message was received successfuly the TCS looks for the trs in the 
	             * server_list and deletes it if it exist in server_list. Otherwise the status
	             * is NOK.*/

	            stopTranslating(server_list, buffer, repply);
	            
	            error = sendto(user, repply, strlen(repply), 0, (struct sockaddr*) &clientaddr, addrlen);

	            if(error == -1)
	            {
	              	perror("An error occurred on sendto");
	              	finishProgram(&user, server_list);
	                return EXIT_FAILURE;
	            }    
	            
	        }
	    }
	    else if(FD_ISSET(0, &rfds))
	   	{
	   		scanf("%s", repply);

	   		if(!strcmp(repply, "exit"))
	   			break;
	   	}

    }

    finishProgram(&user, server_list);
   
    return EXIT_SUCCESS;

}     
