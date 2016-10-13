#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "util.h"
#include "TRS.h"

/* FIXME when you add util.c here */

#define MIN(A,B) (A < B ? A : B)

static int interrupted = 0;

void handle_sigint(int signal) {
    interrupted = 1;
}

int main(int argc, char *argv[])
{
    const char *language = NULL;
    unsigned TRS_port = 59000u;
    unsigned TCS_port = TCS_PORT;
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
    unsigned long bytes_sent = 0;
    unsigned long bytes_received = 0;
    struct timeval tv;

    tv.tv_sec  = 5;
    tv.tv_usec = 0;

    setsockopt(TCS_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));
    setsockopt(TCS_socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(struct timeval));

    gethostname(buffer, BUFFER_SIZE);
    TRS_ptr = gethostbyname(buffer);

    if (TRS_ptr == NULL) {
        perror("Failed to connect to TCS");
        return 0;
    }

    TRS_addr = (struct in_addr*) TRS_ptr->h_addr_list[0];

    /* Prepare the message we need to send to register the language */
    memset((void*)buffer, 0, sizeof(buffer));
    sprintf(buffer, "%s %s %s %u\n", deregister ? "SUN" : "SRG", language, inet_ntoa(*TRS_addr), TRS_port);

    if ((TCS_ptr = gethostbyname(TCS_name)) == NULL) {
        perror("Failed to connect to TCS");
        return 0;
    }

    memset((void*)&TCS_addr, 0, sizeof(TCS_addr));
    TCS_addr.sin_family = AF_INET;
    TCS_addr.sin_addr.s_addr = ((struct in_addr*)(TCS_ptr->h_addr_list[0]))->s_addr;
    TCS_addr.sin_port = htons((u_short) TCS_port);

    /* Make sure we send the entire buffer and not just part of it */
    while (bytes_sent < strlen(buffer)) {
        int received = sendto(TCS_socket, buffer + bytes_sent, strlen(buffer + bytes_sent), 0,
                             (struct sockaddr*)&TCS_addr, addrlen);
        if (received == -1) {
            perror("Failed to send message to TCS");
            return 0;
        }
        bytes_sent += received;
    }

    /* FIXME We need to maybe check if we received everything? */
    bytes_received = recvfrom(TCS_socket, buffer, sizeof(buffer), 0,
                              (struct sockaddr*)&TCS_addr, &addrlen);

    if (bytes_received == -1) {
        perror("Failed to receive TCS reply");
        return 0;
    }

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

    signal(SIGINT, handle_sigint);

    while (running && !interrupted) {
        fd_set input_sources;
        FD_ZERO(&input_sources);
        FD_SET(fileno(stdin), &input_sources);
        FD_SET(TRS_socket, &input_sources);

        if (select(TRS_socket + 1, &input_sources, NULL, NULL, NULL) == -1) {
            if (errno == EINTR) {
                continue; /* hopefully this is just a SIGINT and we can leave now */
            }
            else {
                perror("Something went terribly wrong in select");
                break;
            }
        }

        if(FD_ISSET(TRS_socket, &input_sources)){ /* FIXME */
            struct sockaddr_in client_addr;
            unsigned client_len;
            int client_socket = accept(TRS_socket, (struct sockaddr*)&client_addr, &client_len);
            char buffer[BUFFER_SIZE];
            char request_type = '\0';

            memset(buffer, '\0', sizeof(buffer));
            
            if (client_socket == -1) {
                perror("Failed to create socket for client connection");
                return;
            }

            request_type = get_request_type(client_socket);

            printf("%c\n", request_type);

            // FIXME
            if (request_type == 't') {
                handle_text_translation(client_socket);
            } else if (request_type == 'f') {
                handle_file_translation(client_socket);
            } else {
                report_invalid_request(client_socket);
            }

            sleep(5);
            close(client_socket);
        } else if (FD_ISSET(fileno(stdin), &input_sources)) {
            char buffer[BUFFER_SIZE];
            fgets(buffer, sizeof(buffer), stdin);
            running = strcmp(buffer, "exit\n");
        }
    }

    close(TRS_socket);
}

int get_translation(char const *untranslated, char *translated, char const *filename) {
    FILE *translation_file = fopen(filename, "r"); 
    char buffer[32];
    int got_translation = 0;
    if (translation_file == NULL) {
        perror("Failed to find translation file");
        return 0;
    }
    memset((void*)buffer, '\0', sizeof(buffer));
    while (!feof(translation_file)) {
        fscanf(translation_file, "%s %s\n", buffer, translated);
        if (!strcmp(buffer, untranslated)) {
            got_translation = 1;
            break;
        }
    }
    fclose(translation_file);
    return got_translation;
} 

FILE *get_image_translation(char const *filename, char *new_filename, unsigned long *new_file_size) {
    FILE *translated_file = NULL;
    if (!get_translation(filename, new_filename, "file_translation.txt")) {
        return NULL;
    }
    translated_file = fopen(new_filename, "rb");
    fseek(translated_file, 0, SEEK_END);
    *new_file_size = ftell(translated_file);
    rewind(translated_file);
    return translated_file;
}

int get_text_translation(char const *untranslated, char *translated) {
    return get_translation(untranslated, translated, "text_translation.txt");
}

void report_invalid_request(int client_socket) {
    if (client_socket != -1) {
        safe_write(client_socket, "TRR ERR\n", sizeof("TRR ERR"));
    }
}

char get_request_type(int client_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_read = 0;
    memset((void*)buffer, '\0', sizeof(buffer));
    if (!read_until_space(client_socket, buffer, sizeof(buffer)) || strcmp(buffer, "TRQ")) {
        report_invalid_request(client_socket);
        return '\0';
    }

    if (!read_until_space(client_socket, buffer, sizeof(buffer)) || (buffer[0] != 't' && buffer[0] != 'f')) {
        report_invalid_request(client_socket);
        return '\0';
    }
    return *buffer;
}

void handle_text_translation(int client_socket) {
    char response[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    size_t response_len = 0;
    int num_words = 0;
    size_t bytes_written = 0;

    if (!read_until_space(client_socket, buffer, sizeof(buffer))) {
        report_invalid_request(client_socket);
        return;
    }

    num_words = atoi(buffer);
    if (num_words > MAX_WORDS_PER_REQUEST) {
        report_invalid_request(client_socket);
        return;
    }
    response_len = sprintf(response, "TRR t %d", num_words);
    while (num_words-- > 1) {
        char translated_word[MAX_WORD_LEN+1];
        if (!read_until_space(client_socket, buffer, MAX_WORD_LEN)) {
            report_invalid_request(client_socket);
            return;
        }
        if (!get_text_translation(buffer, translated_word)) {
            strcpy(response, "TRR NTA");
            break;
        }
        strcat(response, " ");
        strcat(response, translated_word);
    }

    if (num_words == 0) { /* Last word ends in a newline*/
        char translated_word[MAX_WORD_LEN+1];
        if (!read_until_newline(client_socket, buffer, MAX_WORD_LEN)) {
            report_invalid_request(client_socket);
            return;
        }
        if (!get_text_translation(buffer, translated_word)) {
            strcpy(response, "TRR NTA");
        } else {
            strcat(response, " ");
            strcat(response, translated_word);
        }
    }

    strcat(response, "\n");
    response_len = strlen(response);
    safe_write(client_socket, response, response_len);
}

void handle_file_translation(int client_socket) {
    FILE *untranslated_file = NULL;
    FILE *translated_file = NULL;
    char untranslated_filename[BUFFER_SIZE];
    char translated_filename[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    unsigned long untranslated_file_size = 0;
    unsigned long translated_file_size = 0;
    size_t bytes_read = 0;
    size_t bytes_written = 0;

    if (!read_until_space(client_socket, untranslated_filename, sizeof(untranslated_filename))) {
        report_invalid_request(client_socket);
        return;
    }

    puts(untranslated_filename); // FIXME

    if (!read_until_space(client_socket, buffer, sizeof(buffer))) {
        report_invalid_request(client_socket);
        return;
    }
    
    untranslated_file_size = atoi(buffer);
    printf("%lu\n", untranslated_file_size); // FIXME

    untranslated_file = fopen(untranslated_filename, "wb");
    bytes_read = 0;

    if(untranslated_file != NULL){
        while(1){
            int received = read(client_socket, buffer, MIN(sizeof(buffer), untranslated_file_size - bytes_read));
            if(received == -1){
                perror("Failed to receive file");
                return;
            }
            bytes_read += received;
            bytes_written = 0;
            while (bytes_written < received) {
                bytes_written += fwrite(buffer + bytes_written, 1, received - bytes_written, untranslated_file);
            }
            if(bytes_read >= untranslated_file_size)
                break;
        }
        fclose(untranslated_file);
    }
    else {
        perror("Failed to receive file");
    }

    translated_file = get_image_translation(untranslated_filename, translated_filename, &translated_file_size);

    if (translated_file == NULL) {
        char const *response = "TRR NTA\n";
        write(client_socket, response, strlen(response));
    }
    else {
        char response[BUFFER_SIZE];
        unsigned long response_size = sprintf(response, "TRR f %s %lu ", translated_filename, translated_file_size);

        bytes_written = 0;
        while (bytes_written < response_size)
            bytes_written += write(client_socket, response, response_size);

        bytes_written = 0;
        bytes_read = 0;
        while (bytes_written < translated_file_size) {
            response_size = fread(response, 1, BUFFER_SIZE, translated_file);
            bytes_written += write(client_socket, response, response_size);
        }

        while (write(client_socket, "\n", 1) != 1) continue;

        fclose(translated_file);
    }
}
