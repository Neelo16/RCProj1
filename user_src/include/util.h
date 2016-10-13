#ifndef UTIL_H
#define UTIL_H

#define GROUPNUMBER 15
#define PORT 58000+GROUPNUMBER

#define MIN(A, B) (A < B ? A : B)

void exitMsg(const char *msg);
void *safeMalloc(size_t size);

/* Reads from file descriptor until there is no space left in the buffer or the character to_read is found. */
/* Returns 1 on success (it found the intended character), and 0 otherwise (if to_read was not found after  */
/* buffer_size bytes or if there was an error in read)                                                      */
/* errno is set if an error occured in read and can be checked                                              */
int read_until_char(int fd, char *buffer, size_t buffer_size, char to_read);

/* Specific case of read_until_char for space (' ')                                                         */
int read_until_space(int fd, char *buffer, size_t buffer_size);

/* Specific case of read_until_char for space ('\n')                                                        */
int read_until_newline(int fd, char *buffer, size_t buffer_size);

#endif
