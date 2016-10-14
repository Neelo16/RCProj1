#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "TCS.h"
#include "queue.h"
#include "util.h"


int getBufferLanguage(char buffer[], char language2[])
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
		return 0;	
	}
 
    /* The request is not properly formatted if the serverFD
     * doesnt add any language or adds more than one.*/
    if((aux == 0 || aux > 1) && !strcmp(language, "UNQ"))
        strcpy(language2, "UNR ERR\n");

    else if( aux != 3 && !strcmp(language,"SRG"))
        strcpy(language2,"SRR ERR\n"); 

    else if( aux != 3 && !strcmp(language,"SUN"))
        strcpy(language2,"SUR ERR\n");

    else
    {
        language = strtok(NULL, " \n"); /*language*/
        if(language != NULL){
        	strcpy(language2,language);
        	return 1;
        }
    }
    return 0;
}

void getTRSInfo(trs_list list, char* language, char reply[])
{
	/* Receives the language requested by the serverFD and finds the TRS server*/

	int replyLen = 0;
    trs_item trs = (trs_item) safeMalloc(sizeof(struct trsItem));
	

    trs = findTRS(list, language);

    /* the language doesn't exist in the server_list */
    if( trs == NULL)
    {
    	destroyTRS(trs);
        strcpy(reply, "UNR EOF\n");
    }

	/* The TRS was found */
    else
    {
    	replyLen = sprintf(reply, "UNR ");
    	replyLen = sprintf(reply + replyLen,"%s %d\n", getIp(trs),getPort(trs));
    	printf("%s\n",reply);
    }    
}

void checkTRS(trs_list list, char buffer[], char reply[])
{
	/* Verifies if there's a TRS with the language requested on the buffer.*/

    char language[MAX];
    char* ip;
    char* port;
    trs_item trs = (trs_item) safeMalloc(sizeof(struct trsItem));

    /*error case*/
    if(!getBufferLanguage(buffer, language) || !strcmp(language, "SRR ERR\n")){
    	strcpy(reply,"SRR ERR\n");
    }   
    else
    {
        /* checks if the TRS is already in the server_list*/
        trs = findTRS(list, language);

        /* if not it adds to the list and sends status OK*/
        if(trs == NULL)
        {
            ip = strtok(NULL, " \n");
            port = strtok(NULL, " \n");
           
            trs = createTRS(language, ip, atoi(port));
           
            addTRSItem(list, trs);

            strcpy(reply, "SRR OK\n");   

            printf("+%s %s %s\n",language,ip,port);         
        }
        /* otherwise sends status not ok*/
        else
            strcpy(reply, "SRR NOK\n");
    }
}


void stopTranslating(trs_list list, char buffer[], char reply[])
{
	/* When the TRS stops translating a language, the TCS will verify if it is in the trs_list,
	in which case it removes it from the list. */

    char language[MAX];
    trs_item trs = (trs_item)safeMalloc(sizeof(struct trsItem));

    /* gets language from buffer */
    getBufferLanguage(buffer, language); 
    
    /* error case sends status not ok*/
    if(!strcmp(language, "SUR ERR\n")) 
    {
        strcpy(reply, "SUR NOK\n");

        free(trs);
    }
    else
    {
        trs = findTRS(list, language); /* tries to find TRS in the TRS list*/

        strcpy(reply, "SUR OK\n");

        if(trs == NULL)
            destroyTRS(trs);

        /*if it is found removes it from the list */
        else{
            removeTRS(list, language); 
            printf("-%s %s %d\n",language,trs->ip,trs->port);
        }
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
	char reply[MAX];
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
    	exitMsg("An error occurred in socket");

    memset((void*)&serveraddr, (int)'\0',sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);
    
    error = bind(serverFD, (struct sockaddr*)&serveraddr, sizeof(serveraddr));

    if(error == -1)
    	exitMsg("An error occurred in bind");


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
	        /*printf("%s",buffer);*/

	        if(!strncmp(buffer, "ULQ", 3))
	        {
	        	printf("List request: %s %d\n",inet_ntoa(clientaddr.sin_addr),ntohs(clientaddr.sin_port));
	        	/* If the format of the buffer is not correct. */
	            if(strlen(buffer) > 4) 
	            {
	                strcpy(reply,"URR\n");
	                error = sendto(serverFD, reply, 4, 0, (struct sockaddr*) &clientaddr, addrlen);

	                if(error == -1)
	                	finishProgram(&serverFD, server_list, "An error occurred on sendto");

	            }
	            /*There are no TRS in the list*/
	            else if( sizeList(server_list) == 0) 
	            {
	                strcpy(reply,"ULR EOF\n");
	                error = sendto(serverFD, reply, 8, 0, (struct sockaddr*) &clientaddr, addrlen);

	                if(error == -1)
	                	finishProgram(&serverFD, server_list, "An error occurred on sendto");
	            }
	            else
	            {
	            	/* gets the languages in the TRS list*/
	                listLanguages(server_list, reply); 
	                error = sendto(serverFD, reply, strlen(reply), 0, (struct sockaddr*) &clientaddr, addrlen);

	                if(error == -1)
	                	finishProgram(&serverFD, server_list, "An error occurred on sendto");
	            }
	        }
	        else if(!strncmp(buffer, "UNQ", 3))
	        {
	            /* gets language requested from the serverFD */
	            getBufferLanguage(buffer, language); 
	            
	            /* looks for the language in TRS list and returns its info */
	            getTRSInfo( server_list, language, reply);
	            error = sendto(serverFD, reply, strlen(reply), 0, (struct sockaddr*) &clientaddr, addrlen);

	            if(error == -1)
	              	finishProgram(&serverFD, server_list, "An error occurred on sendto");
	            
	        }
	        else if(!strncmp( buffer, "SRG", 3))
	        {
	            /*confirms if the TRS is in server_list*/
	            checkTRS(server_list, buffer, reply);

	            /*sends status to TRS */
	            error = sendto(serverFD, reply, strlen(reply), 0, (struct sockaddr*) &clientaddr, addrlen);

	            if(error == -1)
	              	finishProgram(&serverFD, server_list, "An error occurred on sendto");

	        }
	        else if(!strncmp( buffer, "SUN", 3))
	        {
	            /* if the message was received successfuly the TCS looks for the TRS in the 
	             * server_list and deletes it if it exist in server_list. Otherwise the status
	             * is NOK.*/
	            stopTranslating(server_list, buffer, reply);
	            
	            error = sendto(serverFD, reply, strlen(reply), 0, (struct sockaddr*) &clientaddr, addrlen);

	            if(error == -1)
	              	finishProgram(&serverFD, server_list, "An error occurred on sendto");
	            
	        }
	    }
	    /* exit command*/
	    else if(FD_ISSET(0, &rfds)) 
	   	{
	   		scanf("%s", reply);

	   		if(!strcmp(reply, "exit"))
	   			break;
	   	}
    }

    close(serverFD);
   
    return EXIT_SUCCESS;
}     
