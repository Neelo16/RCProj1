#ifndef __user_h__
#define __user_h__

#include <arpa/inet.h>
#include <string.h>

#define GROUPNUMBER 0
#define DEFAULTPORT 50000+GROUPNUMBER
#define DEFAULTADDR "127.0.0.1"

#define BUFFSIZE 512
#define CMDSIZE 128

#define EXITCMD "exit"
#define LISTCMD "list"

#define SENDULQ "ULQ"
#define SENDULQSIZE strlen(SENDULQ)

typedef struct UDPHandler{
	struct sockaddr_in client;
	char buffer[BUFFSIZE];
	int socket,clientLen;
} *UDPHandler_p;

void exitMsg(const char *msg);
void list(UDPHandler_p TCSHandler);
void clean(UDPHandler_p handler);

#endif