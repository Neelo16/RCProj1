#ifndef __TR_SERVER__
#define __TR_SERVER__

#include <stdlib.h>

typedef struct tr_server* trServer;

struct tr_server
{
	char language[20]; //can i do this?
	const char* hostname;
	const int port;
	int n;	
};

char* listLanguages(Queue q);

#endif
