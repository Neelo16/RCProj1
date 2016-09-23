#ifndef QITEM
#define QITEM
#include "tr_server.h"

#define QUEUE_KEY(A) (A->language)
/*#define Q_EQU(A,B) (A == B)
#define Q_LESS(A,B) (A < B)
#define Q_MORE(A,B) (A > B)*/
#define NULL_Q_ITEM NULL

typedef trServer Q_item;
typedef const int Q_KEY;

void deleteQ_item(Q_item i);
void showQ_item(Q_item i);


#endif
