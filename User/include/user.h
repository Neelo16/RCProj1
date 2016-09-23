#ifndef __user_h__
#define __user_h__

#include <arpa/inet.h>
#include <string.h>

#define GROUPNUMBER 0
#define DEFAULTPORT 50000+GROUPNUMBER
#define DEFAULTADDR "127.0.0.1"

#define BUFFSIZE 512
#define CMDSIZE 128

#define EXITCMD "exit\n"
#define LISTCMD "list\n"
#define REQCMD  "request "

#define WORDSIZE 20 /* Each words has WORDSIZE chars max */
#define SENDULQ "ULQ"
#define SENDULQSIZE strlen(SENDULQ)

typedef struct UDPHandler{
	struct sockaddr_in client;
	char buffer[BUFFSIZE];
	int socket;
	unsigned int clientLen;
} *UDPHandler_p;

typedef struct TCPHandler{
	struct sockaddr_in client,server;
	char buffer[BUFFSIZE];
	int clientFD;
} *TCPHandler_p;

void exitMsg(const char *msg);
void list(UDPHandler_p TCSHandler);
void request(UDPHandler_p TCSHandler, char *cmd);
void clean(UDPHandler_p handler);
void splitArgs(char *s, char **result);
void printList(UDPHandler_p TCSHandler);

int stringIn(const char *s1, const char *s2);
int spaceNumber(const char *s1);

#endif