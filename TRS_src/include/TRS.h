#ifndef __RC_2016_TRC__
#define __RC_2016_TRC__

#define BUFFER_SIZE 2048
#define GROUP_NUMBER 15
#define TCS_PORT 58000+GROUP_NUMBER

#define MAX_WORD_LEN 30
#define MAX_WORDS_PER_REQUEST 10

/* Register the server's language in the TCS. Returns 0 on failure, and a nonzero value otherwise. */
int register_language(unsigned TRS_port, char const *TCS_name, unsigned TCS_port, char const *language, int deregister);

void handle_requests(int TRS_port);

/* Get translation for the word contained in untranslated, and place the result in translated.     */
/* Returns 0 on failure, and a nonzero value otherwise.                                            */
int get_text_translation(char const *untranslated, char *translated);

/* Get image translation for the file named filename, and return the corresponding file pointer.   */
/* Returns NULL on failure. new_filename will be modified to contain the returned filename, and    */
/* new_file_size to contain the number of bytes in the new file.                                   */
FILE *get_image_translation(char const *filename, char *new_filename, size_t *new_file_size);

/* Get translation for the word contained in untranslated, and place the result in translated. */
/*  */
int get_translation(char const *untranslated, char *translated, char const *filename);

void report_invalid_request(int client_socket);

void handle_text_translation(int client_socket);

void handle_file_translation(int client_socket);

char get_request_type(int client_fd);

#endif
