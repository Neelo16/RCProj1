#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

#include "TRS.h"

int main(int argc, char *argv[])
{
    const char *language = NULL;
    unsigned port = 59000u;
    unsigned TCS_port = 58000 + GROUP_NUMBER;
    char TCS_name[BUFFER_SIZE];
    gethostname(TCS_name, BUFFER_SIZE);

    /* Make sure we get enough arguments */
    if (argc < 2) {
        fprintf(stderr, "%s%s%s%s\n", argv[0], ": Insufficient arguments\n\t"
                        "Usage: ", argv[0] ," language [-p TRSport] [-n TCSname] [-e TCSport]");
        return EXIT_FAILURE;
    }

    /* Loop over the command line arguments to set the appropriate options */
    {
        int option = -1;
        while ((option = getopt(argc, argv, "pen")) != -1) {
            switch (option) {
                case 'p':
                    port = atoi(argv[optind]);
                    break;
                case 'e':
                    TCS_port = atoi(argv[optind]);
                    break;
                case 'n':
                    strncpy(TCS_name, argv[optind], BUFFER_SIZE);
                    TCS_name[BUFFER_SIZE - 1] = '\0';
                    break;
            }
        }
    }

    printf("port: %u\nTCS port: %u\nTCS name: %s\n", port, TCS_port, TCS_name);
    return EXIT_SUCCESS;
}
