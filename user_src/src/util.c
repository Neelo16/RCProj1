#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
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

int read_until_char(int fd, char *buffer, size_t buffer_size, char to_read) {
    char read_char = '\0';
    size_t bytes_read = 0;
    errno = 0;
    while (bytes_read < buffer_size) {
        int received = read(fd, &read_char, 1);
        if (received == -1) {
            return 0;
        }
        if (read_char == to_read) {
            *buffer = '\0'; /* Terminate the string and return OK */
            return 1;
        } else {
            *buffer = read_char; /* Store the received message */
            buffer += received;  /* until it's over            */
        }
    }
    return 0;
}

inline int read_until_space(int fd, char *buffer, unsigned long buffer_size) {
    return read_until_char(fd, buffer, buffer_size, ' ');
}

inline int read_until_newline(int fd, char *buffer, size_t buffer_size) {
    return read_until_char(fd, buffer, buffer_size, '\n');
}
