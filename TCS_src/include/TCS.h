#ifndef __RC_2016_TCS__
#define __RC_2016_TCS__

#include "queue.h"

#define MAX 1024

/* Gets language from buffer. language2 will have the result and th errors 
to be sent*/
void getBufferLanguage(char buffer[], char language2[]);

/*Receives the language requested by the serverFD and finds the TRS server*/
void getTRSInfo(trs_list list, char* language, char reply[]);

/*Verefies if a trs exists with a language requested on the buffer.*/
void checkTRS(trs_list list, char buffer[], char reply[]);

/* When the trs stops translating a language, ths tcs will verify if it is in the trs_list
and if it is in fact it removes from the list. */
void stopTranslating(trs_list list, char buffer[], char reply[]);

/*Finishes the program properly.*/
void finishProgram(int* user, trs_list list, const char* error);

#endif
