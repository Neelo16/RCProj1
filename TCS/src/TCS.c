#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <TCS.h>

#define PORT 58000

int main(int argc, const char *argv[])  {

    int user, trs;
    struct hostent *hostptr;
    struct sockaddr_in serveraddr, clientaddr;
    int addrlen;
    int port;
    char buffer[100];
    
    /* Port assignment */
    if( sizeof(argv) > 1){
        port = atoi(argv[2]);
    }else{
        port = PORT;
    }

    /*USER*/
    
    user = socket(AF_INET, SOCK_DGRAM,0);

    memset((void*)&serveraddr, (int)'\0',sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((u_short)port);
    
    bind(user, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    addrlen = sizeof(clientaddr);
    
 
    recvfrom(user, buffer,sizeof(buffer),0,(struct sockaddr*)&clientaddr, &addrlen);
    
    /*
    while(!strcmp(buffer, "exit")) {
        recvfrom(user, buffer, sizeof(buffer), 0, (struck sockaddr*)&clientaddr,&addrlen);

        switch{}
          sendto(fd,)
    }
    */
}
