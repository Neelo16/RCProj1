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

    if (!register_language(TRS_port, TCS_name, TCS_port, language)) {
        fprintf(stderr, "Failed to register language %s\n", language);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int register_language(unsigned TRS_port, char const *TCS_name, unsigned TCS_port, char const *language) {
    int TCS_socket = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in TCS_addr;
    struct in_addr *TRS_addr;
    struct hostent *TCS_ptr = NULL;
    struct hostent *TRS_ptr = NULL;
    unsigned addrlen = sizeof(TCS_addr);
    char buffer[BUFFER_SIZE];
    int result = 0;
    int bytes_sent = 0;
    int bytes_received = 0;

    gethostname(buffer, BUFFER_SIZE);
    TRS_ptr = gethostbyname(buffer);

    if ((TCS_ptr = gethostbyname(TCS_name)) == NULL || TRS_ptr == NULL) {
        perror("Failed to connect to TCS");
        return 0;
    }
    TRS_addr = (struct in_addr*) TRS_ptr->h_addr_list[0];

    memset((void*)buffer, 0, sizeof(buffer));
    sprintf(buffer, "SRG %s %s %u\n", language, inet_ntoa(*TRS_addr), TRS_port);

    memset((void*)&TCS_addr, 0, sizeof(TCS_addr));
    TCS_addr.sin_family = AF_INET;
    TCS_addr.sin_addr.s_addr = ((struct in_addr*)(TCS_ptr->h_addr_list[0]))->s_addr;
    TCS_addr.sin_port = htons((u_short) TCS_port);

    while (bytes_sent < strlen(buffer)) {
        bytes_sent += sendto(TCS_socket, buffer + bytes_sent, strlen(buffer + bytes_sent), 0,
                             (struct sockaddr*)&TCS_addr, addrlen);
    }

    bytes_received = recvfrom(TCS_socket, buffer, sizeof(buffer), 0,
                              (struct sockaddr*)&TCS_addr, &addrlen);

    buffer[bytes_received] = '\0';

    if (!strcmp(buffer, "OK\n")) {
        result = 1;
    } else if(!strcmp(buffer, "NOK\n") || !strcmp(buffer, "NERR\n")) {
        result = 0;
    }

    puts("Start of buffer");
    fputs(buffer, stdout);
    puts("End of buffer");

    close(TCS_socket);
    return result;
}
