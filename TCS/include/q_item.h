#ifndef QITEM
#define QITEM
#include "cheques.h"

#define QUEUE_KEY(A) (A->ref)
#define Q_EQU(A,B) (A == B)
#define Q_LESS(A,B) (A < B)
#define Q_MORE(A,B) (A > B)
#define NULL_Q_ITEM NULL

typedef Cheque Q_item;
typedef long unsigned int Q_KEY;

void deleteQ_item(Q_item i);
void showQ_item(Q_item i);


#endif