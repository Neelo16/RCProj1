#include <stdio.h>
#include <stdlib.h>
#include "util.h"

void exitMsg(const char *msg){
	perror(msg);
	exit(1);
}

void *safeMalloc(size_t size){
	void *answer;
	if((answer = malloc(size)) == NULL)
		exitMsg("Error in safeMalloc");
	return answer;
}
