#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

#include "TRS.h"

int main(int argc, char *argv[])
{
    const char *language = NULL;
    unsigned TRS_port = 59000u;
    unsigned TCS_port = 58000 + GROUP_NUMBER;
    char TCS_name[BUFFER_SIZE];
    gethostname(TCS_name, BUFFER_SIZE);

    /* Make sure we get enough arguments */
    if (argc < 2) {
        fprintf(stderr, "%s%s%s%s\n", argv[0], ": Insufficient arguments\n\t"
                        "Usage: ", argv[0] ," language [-p TRSport] [-n TCSname] [-e TCSport]");
        return EXIT_FAILURE;
    }

    language = argv[1];

    /* Loop over the command line arguments to set the appropriate options */
    {
        int option = -1;
        while ((option = getopt(argc, argv, "p:e:n:")) != -1) {
            switch (option) {
                case 'p':
                    TRS_port = atoi(optarg);
                    break;
                case 'e':
                    TCS_port = atoi(optarg);
                    break;
                case 'n':
                    strncpy(TCS_name, optarg, BUFFER_SIZE);
                    TCS_name[BUFFER_SIZE - 1] = '\0';
                    break;
            }
        }
    }

    printf("TRS port: %u\nTCS port: %u\nTCS name: %s\n", TRS_port, TCS_port, TCS_name);

    if (!register_language(TRS_port, TCS_name, TCS_port, language, 0)) {
        fprintf(stderr, "Failed to register language %s\n", language);
        return EXIT_FAILURE;
    }

    handle_requests(TRS_port);

    register_language(TRS_port, TCS_name, TCS_port, language, 1);

    return EXIT_SUCCESS;
}

int register_language(unsigned TRS_port, char const *TCS_name, unsigned TCS_port, char const *language, int deregister) {
    int TCS_socket = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in TCS_addr;
    struct in_addr *TRS_addr;
    struct hostent *TCS_ptr = NULL;
    struct hostent *TRS_ptr = NULL;
    unsigned addrlen = sizeof(TCS_addr);
    char buffer[BUFFER_SIZE];
    int result = 0;
    size_t bytes_sent = 0;
    size_t bytes_received = 0;

    gethostname(buffer, BUFFER_SIZE);
    TRS_ptr = gethostbyname(buffer);

    if ((TCS_ptr = gethostbyname(TCS_name)) == NULL || TRS_ptr == NULL) {
        perror("Failed to connect to TCS");
        return 0;
    }
    TRS_addr = (struct in_addr*) TRS_ptr->h_addr_list[0];

    /* Prepare the message we need to send to register the language */
    memset((void*)buffer, 0, sizeof(buffer));
    sprintf(buffer, "%s %s %s %u\n", deregister ? "SUN" : "SRG", language, inet_ntoa(*TRS_addr), TRS_port);

    memset((void*)&TCS_addr, 0, sizeof(TCS_addr));
    TCS_addr.sin_family = AF_INET;
    TCS_addr.sin_addr.s_addr = ((struct in_addr*)(TCS_ptr->h_addr_list[0]))->s_addr;
    TCS_addr.sin_port = htons((u_short) TCS_port);

    /* Make sure we send the entire buffer and not just part of it */
    while (bytes_sent < strlen(buffer)) {
        bytes_sent += sendto(TCS_socket, buffer + bytes_sent, strlen(buffer + bytes_sent), 0,
                             (struct sockaddr*)&TCS_addr, addrlen);
    }

    /* FIXME We need to maybe check if we received everything? */
    bytes_received = recvfrom(TCS_socket, buffer, sizeof(buffer), 0,
                              (struct sockaddr*)&TCS_addr, &addrlen);

    /* FIXME change this filthy hack */
    buffer[bytes_received < BUFFER_SIZE ? bytes_received : BUFFER_SIZE - 1] = '\0';

    /* FIXME I should probably change these conditionals to something else */
    if (!strcmp(buffer, deregister ? "SUR OK\n" : "SRR OK\n")) {
        result = 1;
    } else if(!strcmp(buffer, deregister ? "SUR NOK\n" : "SRR NOK\n") || !strcmp(buffer, deregister ? "SUR NERR" : "SRR NERR\n")) {
        result = 0;
    }

    close(TCS_socket);
    return result;
}

void handle_requests(int TRS_port) {
    int TRS_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in TRS_addr;
    int running = 1;

    memset((void*)&TRS_addr, '\0', sizeof(TRS_addr));
    TRS_addr.sin_family = AF_INET;
    TRS_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    TRS_addr.sin_port = htons((u_short) TRS_port);

    if (bind(TRS_socket, (struct sockaddr*)&TRS_addr, sizeof(TRS_addr)) == -1) {
        perror("Failed to bind address");
        return;
    }

    if (listen(TRS_socket, 5) == -1) {
        perror("Error in listen");
        return;
    }

    while (running) {
        struct sockaddr_in client_addr;
        unsigned client_len;
        int client_socket = accept(TRS_socket, (struct sockaddr*)&client_addr, &client_len);
        char buffer[BUFFER_SIZE];
        size_t bytes_read = 0;
        size_t bytes_written = 0;
        char *argument = NULL;

        memset(buffer, '\0', sizeof(buffer));
        
        if (client_socket == -1) {
            /* FIXME */
        }

        /* Ensure we receive at least enough bytes to determine if we have a valid request   */
        /* No way to know if we received the whole thing yet as we don't know if it's a file */
        /* or a list of words. */
        while (bytes_read < sizeof("TRQ f")) {
            bytes_read += read(client_socket, buffer, sizeof("TRQ f") - bytes_read);
        }

        argument = strtok(buffer, " ");

        if (!strcmp(argument, "TRQ")) {
            argument = strtok(NULL, " ");
            if (!strcmp(argument, "t")) {
                char response[512];
                int response_len = 0;
                int num_words = 0;
                bytes_written = 0;

                /* Now that we know this must be a string terminated by a newline, we can check if */
                /* we received the entire request */

                while (buffer[bytes_read - 1] != '\n') {
                    bytes_read += read(client_socket, buffer + bytes_read, sizeof(buffer) - bytes_read);
                }
                /* Get rid of the newline character at the end and turn this into a null-terminated string */
                buffer[bytes_read - 1] = '\0';

                argument = strtok(NULL, " ");
                num_words = atoi(argument);
                response_len = sprintf(response, "TRR t %d", num_words);
                while (num_words-- > 0) {
                    char translated_word[31];
                    argument = strtok(NULL, " ");
                    if (!get_text_translation(argument, translated_word)) {
                        strcpy(response, "TRR NTA");
                        break;
                    }
                    strcat(response, " ");
                    strcat(response, translated_word);
                }
                strcat(response, "\n");
                response_len = strlen(response);
                while (bytes_written < response_len) {
                    bytes_written += write(client_socket, response, response_len);
                }
            } else if (!strcmp(argument, "f")) {
                /* TODO save data to a file */
                /* TODO make sure we received everything we need */
                FILE *new_file = NULL;
                FILE *old_file = NULL;
                char filename[BUFFER_SIZE];
                char new_filename[BUFFER_SIZE];
                size_t new_file_size = 0;
                size_t old_file_size = 0;
                bytes_read = 0;

                memset(filename, '\0', sizeof(filename));
                memset(new_filename, '\0', sizeof(new_filename));
                memset(buffer, '\0', sizeof(buffer));

                while (1) {
                    bytes_read += read(client_socket, filename + bytes_read, 1);
                    if (bytes_read && filename[bytes_read - 1] == ' ')
                        break;
                }
                filename[bytes_read-1] = '\0';
                printf("Filename:%s|\n",filename);
                bytes_read = 0;
                while (1) {
                    bytes_read += read(client_socket, buffer + bytes_read, 1);
                    if (bytes_read && buffer[bytes_read - 1] == ' ')
                        break;
                }
                buffer[bytes_read] = '\0';
                old_file_size = atoi(buffer);

                old_file = fopen(filename, "wb");
                bytes_read = 0;

                if(old_file != NULL){
                    while(1){
                        int received = read(client_socket, buffer, sizeof(buffer));
                        if(!received){
                            perror("Erro");
                            return;
                        }
                        bytes_read += received;
                        bytes_written = 0;
                        while (bytes_written < received) {
                            bytes_written += fwrite(buffer + bytes_written, 1, received - bytes_written, old_file);
                        }
                        if(bytes_read >= old_file_size)
                            break;
                    }
                    fclose(old_file);
                    printf("received file %s\n     %ld Bytes\n",filename,old_file_size);
                }
                else {
                    printf("Error trying to download this file: %s\n",filename);
                }
                printf("Going to get the translation for %s now\n", filename);
                new_file = get_image_translation(filename, new_filename, &new_file_size);
                printf("Sending %s next\n", new_filename);
                if (new_file == NULL) {
                    char const *response = "TRR NTA\n";
                    write(client_socket, response, strlen(response));
                }
                else {
                    char response[BUFFER_SIZE];
                    size_t response_size = sprintf(response, "TRR f %s %d ", new_filename, new_file_size);

                    bytes_written = 0;
                    while (bytes_written < response_size)
                        bytes_written += write(client_socket, response, response_size);

                    bytes_written = 0;
                    bytes_read = 0;
                    while (bytes_written < new_file_size) {
                        response_size = fread(response, 1, BUFFER_SIZE, new_file);
                        bytes_written += write(client_socket, response, response_size);
                    }

                    puts("sent");

                    fclose(new_file);
                }
            } else {
                goto BAD_FORMAT;
            }
        } else {
BAD_FORMAT:
            write(client_socket, "TRR ERR\n", strlen("TRR ERR\n"));
        }

        sleep(5);
        close(client_socket);
    }

    close(TRS_socket);
}

int get_translation(char const *untranslated, char *translated, char const *filename) {
    FILE *translation_file = fopen(filename, "r");
    char buffer[32];
    int got_translation = 0;
    memset((void*)buffer, '\0', sizeof(buffer));
    while (ftell(translation_file) != SEEK_END ) {
        fscanf(translation_file, "%s %s", buffer, translated);
        if (!strcmp(buffer, untranslated)) {
            got_translation = 1;
            break;
        }
    }
    fclose(translation_file);
    return got_translation;
} 

FILE *get_image_translation(char const *filename, char *new_filename, size_t *new_file_size) {
    FILE *translated_file = NULL;
    puts("In image translation");
    if (!get_translation(filename, new_filename, "file_translation.txt")) {
        return NULL;
    }
    puts("Got translation");
    translated_file = fopen(new_filename, "rb");
    fseek(translated_file, 0, SEEK_END);
    *new_file_size = ftell(translated_file);
    fseek(translated_file, 0, SEEK_SET);
    return translated_file;
}

int get_text_translation(char const *untranslated, char *translated) {
    return get_translation(untranslated, translated, "text_translation.txt");
}

