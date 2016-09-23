#include "tr_server_list.h"

l_trServer newTRSList()
{
	l_trServer list = (l_trServer) malloc(sizeof(struct l_tr_server));
	
	list->q = createQueue();
	
	return list;
}


tr_server popTRS(l_trServer list)
{
	tr_server trs = (tr_server) q_pop(list->q);
	
	return trs;
}


void insertTRS(l_trServer list, tr_server trs)
{
	q_push(list->q, trs);
}


tr_server findTRS(l_trServer list, char language)
{
	return (tr_server) q_find(list->q, language);
}


tr_server removeTRS(l_trServer list, char language)
{
	tr_server trs;

	trs = (tr_server) q_remove(list, language);

	return trs;
}


void deleteTRSList(l_trServer list)
{
	tr_server trs;

	while( (trs = q_pop(list) != NULL))
		free(trs);

	deleteQueue(list->q);
	free(list);
}

int totalTRS(l_trServer list)
{
	return list->q->size;
}


