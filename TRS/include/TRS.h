#ifndef __RC_2016_TRC__
#define __RC_2016_TRC__

#define BUFFER_SIZE 2048
#define GROUP_NUMBER 0

/* Register the server's language in the TCS. Returns 0 on failure, and a nonzero value otherwise. */
int register_language(unsigned TRS_port, char const *TCS_name, unsigned TCS_port, char const *language, int deregister);

void handle_requests(int TRS_port);

/* Get translation for the word contained in untranslated, and place the result in translated.     */
/* Returns 0 on failure, and a nonzero value otherwise.                                            */
int get_text_translation(char const *untranslated, char *translated);

/* Get image translation for the file named filename, and return the data in the translated file.  */
/* Returns NULL on failure. new_filename will be modified to contain the returned filename, and    */
/* new_file_size to contain the number of bytes in the new file.                                   */
char *get_image_translation(char const *filename, char *new_filename, size_t *new_file_size);

/* Get translation for the word contained in untranslated, and place the result in translated. */
/*  */
int get_translation(char const *untranslated, char *translated, char *filename);

#endif
