#ifndef __user_h__
#define __user_h__

#include <arpa/inet.h>
#include <string.h>

#define BUFFSIZE 2048
#define CMDSIZE 2048

#define EXITCMD "exit\n"
#define LISTCMD "list\n"
#define REQCMD  "request"

#define DEFAULTADDR "127.0.0.1"
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


int request(UDPHandler_p TCSHandler, TCPHandler_p TRSHandler, char *cmd, char **languages, int langNumber);
int list(UDPHandler_p TCSHandler, char ***languages);

int safeSendUDP(UDPHandler_p TCSHandler, const char *toSend, unsigned int toSendLen);
int TCPConnection(TCPHandler_p TRSHandler, const char *ip, const int port, const char *language);
int getLanguages(UDPHandler_p TCSHandler, char ***languages);

int sendUNQ(TCPHandler_p TRSHandler, UDPHandler_p TCSHandler, char **languages, int langName, char **ip, unsigned int *port);
int sendFile(TCPHandler_p TRSHandler, char *filename);
int recvInitialData(TCPHandler_p TRSHandler, char *filename, unsigned long int *size);
void handleFileTranslation(TCPHandler_p TRSHandler, char *filename);
void handleTextTranslation(TCPHandler_p TRSHandler, char **words, char *ip,int N);
void printWordsReceived(char *buffer);

void parseTCSOptions(UDPHandler_p TCSHandler,int argc, char **argv);
int parseTCSUNR(UDPHandler_p TCSHandler, char **ip, unsigned int *port);
char parseRequest(char *cmd, char *filename, char **words,int *langName, int *N);

#endif
