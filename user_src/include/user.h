#ifndef __user_h__
#define __user_h__

#include <arpa/inet.h>
#include <string.h>

#define BUFFSIZE 2048
#define CMDSIZE 2048

#define EXITCMD "exit\n"
#define LISTCMD "list\n"
#define REQCMD  "request "

#define DEFAULTADDR "127.0.0.1"

#define SENDULQ "ULQ\n"
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
	int clientFD;
} *TCPHandler_p;

void cleanLanguagesList(char **languages, int langNumber);
void cleanUDP(UDPHandler_p handler);
void cleanTCP(TCPHandler_p handler);
int safeSendUDP(UDPHandler_p TCSHandler, const char *toSend, unsigned int toSendLen);
int TCPConnection(TCPHandler_p TRSHandler, const char *ip, const int port, const char *language);
void request(UDPHandler_p TCSHandler, TCPHandler_p TRSHandler, char *cmd, char **languages, int langNumber);
int list(UDPHandler_p TCSHandler, char ***languages);
int getLanguages(UDPHandler_p TCSHandler, char ***languages);
int stringIn(const char *s1, const char *s2);
int parseTCSUNR(UDPHandler_p TCSHandler, char **ip, unsigned int *port);

#endif
