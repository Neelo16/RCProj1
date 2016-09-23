#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <TCS.h>
#include <queue.h>

#define PORT 58000
#define MAX 100

int main(int argc, const char *argv[])  {

    int user, trs; //sockets
    struct hostent *hostptr;
    struct sockaddr_in serveraddr, clientaddr;
    int addrlen;
    int port;
    char buffer[MAX];
	char repply[MAX];
    

	//Create Server List
	l_trServer server_list = newTRSList();

    // Port assignment 
    if( sizeof(argv) > 1){
        port = atoi(argv[2]);
    }else{
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
    
 
    recvfrom(user, buffer,sizeof(buffer),0,(struct sockaddr*)&clientaddr, &addrlen);
    
   
    while(!strcmp(buffer, "exit"))
	 {
        //User asks to translate TCS answers
		if(!strncmp(buffer, "UQL", 3))
		{
			if(server_list->size == 0)
			{
				repply = "URL EOF\n";
				sendto(user, repply, sizeof(repply), 0, NULL, 0);
			}
			else if(!strcmp(buffer, "UQL list"))
			{
				
				strcopy(repply,"ULR ");
				repply = strcat(repply, listLanguages(server_list); //FIXME final of repply \n
			
				sendto(user,repply, sizeof(repply), 0, NULL, 0);//FIXME
			}
			else
			{ 
				repply = "UQL ERR\n";
				sendto(user, repply, sizeof(repply), 0, NULL, 0);
			}
		}

		//User asks for the translation of TRS
		if(!strncmp(buffer, "UNQ", 3))
		{

		/*c)
			ir ah lista buscar TRS
			ligar se a ele
			pedir os seus detalhes
		d)
			fica ah espera ate receber IP e port (atençao aos casos de erro)
			envia ao user
		*/
		
	



			recvfrom(usr, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientaddr, &addrlen);
	} 

		
    }
    
