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
#include "util.h"


void getBufferLanguage(char buffer[], char language2[])
{
	/* Gets language from buffer. language2 will have the result and th errors 
	to be sent*/

    int i, aux = 0; 
    char *language;
	
	/*counts the number of spaces received in the buffer to check if it has 
	the right format*/
    for(i = 0 ; buffer[i] != '\0'; i++)
    {
        if(buffer[i] == ' ')
            aux++;
    }

    language = strtok(buffer, " ");

	if(language == NULL){
		fprintf(stderr, "User sent an invalid command\n");
		return;	
	}
 
    /* The format is not formulated correcly if the serverFD
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
	/*Receives the language requested by the serverFD and finds the TRS server*/

	int repplyLen = 0;
    trs_item trs = (trs_item) safeMalloc(sizeof(struct trsItem));
	

    trs = findTRS(list, language);

    /* the language doesnt exist in the server_list */
    if( trs == NULL)
    {
    	destroyTRS(trs);
        strcpy(repply, "UNR EOF\n");
    }

	/* The trs was found */
    else
    {
    	repplyLen = sprintf(repply, "UNR ");
    	repplyLen = sprintf(repply + repplyLen,"%s %d\n", getIp(trs),getPort(trs));
    }    
}

void checkTRS(trs_list list, char buffer[], char repply[])
{
	/*Verefies if a trs exists with a language requested on the buffer.*/

    char language[MAX];
    char* ip;
    char* port;
    trs_item trs = (trs_item) safeMalloc(sizeof(struct trsItem));

    getBufferLanguage(buffer, language);
    
    /*error case*/
    if(!strcmp(language, "SRR ERR\n"))
        strcpy(repply, language);
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
    }
}


void stopTranslating(trs_list list, char buffer[], char repply[])
{
	/* When the trs stops translating a language, ths tcs will verify if it is in the trs_list
	and if it is in fact it removes from the list. */

    char language[MAX];
    trs_item trs = (trs_item)safeMalloc(sizeof(struct trsItem));

    /* gets language from buffer */
    getBufferLanguage(buffer, language); 
    
    /* error case sends status not ok*/
    if(!strcmp(language, "SUR ERR\n")) 
    {
        strcpy(repply, "SUR NOK\n");

        destroyTRS(trs);
    }
    else
    {
        trs = findTRS(list, language); /* tries to find trs in the tres list*/

        strcpy(repply, "SUR OK\n");

        if(trs == NULL)
            destroyTRS(trs);

        /*if it is found removes it from the list */
        else
            removeTRS(list, language); 
    }
}

/*Finishes the program properly.*/
void finishProgram(int* serverFD, trs_list list, const char* error)
{
	if(close(*serverFD) == -1)
		perror("An error occurred on close");
	destroyList(list);
	exitMsg(error);
}

int main(int argc, const char **argv)  {

    struct sockaddr_in serveraddr, clientaddr;
	int serverFD = 0; /* socket fd */
    int port, error, retval;
    int received = 0;
    unsigned int addrlen;
    char buffer[MAX];
	char repply[MAX];
    char language[MAX];
    trs_list server_list;
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);


	/* Create Server List */
	server_list = createList(); 

    /*  Port assignment */
    if( argc > 2 && !strcmp(argv[1], "-p"))
        port = atoi(argv[1]);

    /* in case -p TCSport is omitted */
    else 
        port = PORT;


    /* -----------------------Server---------------------------- */
	
	/* Initialization of the socket with UDP protocol*/    
    serverFD = socket(AF_INET, SOCK_DGRAM,0);

    if(serverFD == -1)
    	exitMsg("An error occurrer on socket");

    memset((void*)&serveraddr, (int)'\0',sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);
    
    error = bind(serverFD, (struct sockaddr*)&serveraddr, sizeof(serveraddr));

    if(error == -1)
    	exitMsg("An error occurrer an error on bind");


    addrlen = sizeof(clientaddr);
	
	/*Program starts*/
    while(1)
    {
    	FD_ZERO(&rfds);
        FD_SET(0, &rfds);
    	FD_SET(serverFD, &rfds);

    	retval = select(serverFD+1, &rfds, NULL, NULL, NULL);

    	if(retval == -1)
    		finishProgram(&serverFD, server_list, "An error occurred on select.");


    	else if( FD_ISSET(serverFD, &rfds))
    	{
    		/*Receives buffer from client*/
	        received = recvfrom(serverFD, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientaddr, &addrlen);
	        
	        if(received == -1)
	            finishProgram(&serverFD, server_list, "An error occurred on recvfrom");
	      
			
            
	        *(buffer + received) = '\0';
	        printf("%s",buffer);

	        if(!strncmp(buffer, "ULQ", 3))
	        {
	        	/* If the format of the buffer is not correct. */
	            if(strlen(buffer) > 4) 
	            {
	                strcpy(repply,"URR\n");
	                error = sendto(serverFD, repply, 4, 0, (struct sockaddr*) &clientaddr, addrlen);

	                if(error == -1)
	                	finishProgram(&serverFD, server_list, "An error occurred on sendto");

	            }
	            /*There are no TRS in the list*/
	            else if( sizeList(server_list) == 0) 
	            {
	                strcpy(repply,"ULR EOF\n");
	                error = sendto(serverFD, repply, 8, 0, (struct sockaddr*) &clientaddr, addrlen);

	                if(error == -1)
	                	finishProgram(&serverFD, server_list, "An error occurred on sendto");
	            }
	            else
	            {
	            	/* gets the languages in the TRS list*/
	                listLanguages(server_list, repply); 
	                error = sendto(serverFD, repply, strlen(repply), 0, (struct sockaddr*) &clientaddr, addrlen);

	                if(error == -1)
	                	finishProgram(&serverFD, server_list, "An error occurred on sendto");
	            }
	        }
	        else if(!strncmp(buffer, "UNQ", 3))
	        {
	            /* gets language requested from the serverFD */
	            getBufferLanguage(buffer, language); 
	            
	            /* looks for the language in trs list and returs its info */
	            getTRSInfo( server_list, language, repply);
	            error = sendto(serverFD, repply, strlen(repply), 0, (struct sockaddr*) &clientaddr, addrlen);

	            if(error == -1)
	              	finishProgram(&serverFD, server_list, "An error occurred on sendto");
	            
	        }
	        else if(!strncmp( buffer, "SRG", 3))
	        {
	            /*confirms if the trs is in server_list*/
	            checkTRS(server_list, buffer, repply);

	            /*sends status to TRS*/
	            error = sendto(serverFD, repply, strlen(repply), 0, (struct sockaddr*) &clientaddr, addrlen);

	            if(error == -1)
	              	finishProgram(&serverFD, server_list, "An error occurred on sendto");

	        }
	        else if(!strncmp( buffer, "SUN", 3))
	        {
	            /* if the message was received successfuly the TCS looks for the trs in the 
	             * server_list and deletes it if it exist in server_list. Otherwise the status
	             * is NOK.*/
	            stopTranslating(server_list, buffer, repply);
	            
	            error = sendto(serverFD, repply, strlen(repply), 0, (struct sockaddr*) &clientaddr, addrlen);

	            if(error == -1)
	              	finishProgram(&serverFD, server_list, "An error occurred on sendto");
	            
	        }
	    }
	    /* exit command*/
	    else if(FD_ISSET(0, &rfds)) 
	   	{
	   		scanf("%s", repply);

	   		if(!strcmp(repply, "exit"))
	   			break;
	   	}
    }

    close(serverFD);
   
    return EXIT_SUCCESS;
}     
