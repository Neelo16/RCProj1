#include "q_item.h"

void showQ_item(Q_item i)
{
	showCheque( (Cheque) i);
}

void deleteQ_item(Q_item i)
{
	deleteCheque( (Cheque) i);
}