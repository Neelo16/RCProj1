#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 5000X
#define BUFFSIZE 512

int main(int argc, char **argv){
	char buffer[BUFFSIZE];
	printf("It is not initialized, but why would i care right? %s\n",buffer);
}