#ifndef UTIL_H
#define UTIL_H

#define GROUPNUMBER 15
#define PORT 58000+GROUPNUMBER

void exitMsg(const char *msg);
void *safeMalloc(size_t size);

#endif
