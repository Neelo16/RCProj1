#ifndef __RC_2016_TRC__
#define __RC_2016_TRC__

#define BUFFER_SIZE 128
#define GROUP_NUMBER 0

/* Register the server's language in the TCS. Returns 0 on failure, and a nonzero value otherwise  */
int register_language(unsigned TRS_port, char const *TCS_name, unsigned TCS_port, char const *language, int deregister);

#endif
