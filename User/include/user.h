#ifndef __user_h__
#define __user_h__

#include <arpa/inet.h>
#include <string.h>

#define BUFFSIZE 512
#define CMDSIZE 128

#define EXITCMD "exit\n"
#define LISTCMD "list\n"
#define REQCMD  "request "

#define GROUPNUMBER 0
#define DEFAULTADDR "127.0.0.1"
#define DEFAULTPORT 50000+GROUPNUMBER

#define SENDULQ "ULQ"
#define SENDULQSIZE strlen(SENDULQ)
#define WORDSIZE 31 /* Each words has WORDSIZE-1 chars max */

typedef struct UDPHandler{
	struct sockaddr_in client;
	char buffer[BUFFSIZE];
	int socket;
	unsigned int clientLen;
} *UDPHandler_p;

typedef struct TCPHandler{
	struct sockaddr_in server; /* Server to connect to */
	char buffer[BUFFSIZE];
	socklen_t serverSize;
	char language[WORDSIZE]; /*Language that is being translated currently */
	int clientFD,connected;
} *TCPHandler_p;

void exitMsg(const char *msg);
int getLanguages(UDPHandler_p TCSHandler, char ***languages);
int list(UDPHandler_p TCSHandler, char ***languages);
void request(UDPHandler_p TCSHandler, char *cmd, char **languages);
void cleanUDP(UDPHandler_p handler);
void cleanTCP(TCPHandler_p handler);
void cleanLanguagesList(char **languages, int langNumber);

int stringIn(const char *s1, const char *s2);

#endif