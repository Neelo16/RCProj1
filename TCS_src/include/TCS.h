#ifndef __RC_2016_TCS__
#define __RC_2016_TCS__

#include "queue.h"

#define PORT 58000
#define MAX 1024

void getBufferLanguage(char buffer[], char language2[]);
void getTRSInfo(trs_list list, char* language, char repply[]);
void checkTRS(trs_list list, char buffer[], char repply[]);
void stopTranslating(trs_list list, char buffer[], char repply[]);
void finishProgram(int* user, trs_list list);

#endif
