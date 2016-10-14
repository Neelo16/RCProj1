#ifndef __RC_2016_TRC__
#define __RC_2016_TRC__

#define BUFFER_SIZE 2048
#define GROUP_NUMBER 15
#define TCS_PORT 58000+GROUP_NUMBER

#define MAX_WORD_LEN 30
#define MAX_WORDS_PER_REQUEST 10

/* Register the server's language in the TCS, or deregisters it, depending on the boolean value in deregister. */
/* Returns 0 on failure, and a nonzero value otherwise.                                                        */
int register_language(unsigned TRS_port, char const *TCS_name, unsigned TCS_port, char const *language, int deregister);

/* Loops and handles any attempts at connecting to the server or exit commands from the console */
void handle_requests(int TRS_port);

/* Get translation for the word contained in untranslated, and place the result in translated.     */
/* Returns 0 on failure, and a nonzero value otherwise.                                            */
int get_text_translation(char const *untranslated, char *translated);

/* Get image translation for the file named filename, and return the corresponding file pointer.   */
/* Returns NULL on failure. new_filename will be modified to contain the returned filename, and    */
/* new_file_size to contain the number of bytes in the new file.                                   */
FILE *get_image_translation(char const *filename, char *new_filename, size_t *new_file_size);

/* Get translation for the word contained in untranslated, and place the result in translated. */
int get_translation(char const *untranslated, char *translated, char const *filename);

/* Reports to the client that they sent an invalid request */
void report_invalid_request(int client_socket);

/* Received a list of at most 10 space-separated words from client_socket, ending in a newline,  */
/* prints them to the screen, sends their translation (if possible) and outputs that translation */
/* to the screen                                                                                 */
void handle_text_translation(int client_socket);

/* Receives a file from the given socket file descriptor and sends its corresponding translation  */
void handle_file_translation(int client_socket);

/* Validates the start of a request and returns the char corresponding to the request type: */
/* t if it's a text translation, f if it's a file translation, and '\0' otherwise           */
char get_request_type(int client_fd);

#endif
