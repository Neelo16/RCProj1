#include "queue.h"

q_link create_q_node(Q_item i)
{
	q_link n = (q_link) malloc(sizeof(struct q_node));

	n->item = i;
	n->next = NULL;

	return n;
}

void destroy_q_node(q_link n)
{
	free(n);
}

Queue createQueue()
{
	Queue q = (Queue) malloc(sizeof(struct queue));

	q->head = NULL;
	q->tail = NULL;
	q->size = 0;

	return q;
}

void q_push(Queue q, Q_item i) /* Insere um elemento no fim da Queue */
{
	q_link n = create_q_node(i);

	if(q->head == NULL)
		q->head = (q->tail = n);
	else
	{
		q->tail->next = n;
		q->tail = n;
	}
	q->size++;
}

void q_show(Queue q)
{
	q_link t;

	for(t = q->head; t != NULL; t = t->next)
		showQ_item(t->item);
	putchar('\n');
}
	
Q_item q_pop(Queue q) /* Remove o primeiro elemento (ou seja, o mais antigo) da Queue */
{
	q_link h;
	Q_item i;

	if(q->head == NULL) 
		return NULL;

	h = q->head;
	i = h->item;
	q->head = h->next;

	destroy_q_node(h);
	q->size--;
	return i;
}

Q_item q_find(Queue q, Q_KEY k) /* Encontra um item e devolve-o, sem que este seja removido da queue */
{
	q_link t;

	for(t = q->head; t != NULL; t = t->next)
		if(Q_EQU(QUEUE_KEY(t->item), k))
			return t->item;

	return NULL_Q_ITEM;
	
}
void deleteQueue(Queue q)
{
	q_link t, aux;

	for(t = q->head; t != NULL; t = aux)
	{
		aux = t->next;
		destroy_q_node(t);
	}
	free(q);
}

Q_item q_remove(Queue q, Q_KEY k) /* Igual ao q_find, mas o item e removido da queue */
{
	q_link t, aux;
	Q_item aux_i;

	for(t = q->head; t != NULL; aux = t, t = t->next)
	{
		if(Q_EQU(QUEUE_KEY(t->item), k))
		{
			if(t == q->head)
				return q_pop(q);

			else if(t == q->tail)
				q->tail = aux;

			aux->next = t->next;	/* Liga o node anterior ao proximo node, */
			aux_i = t->item;		/* de forma a poder eliminar o node pretendido. */
			destroy_q_node(t);
			q->size--;
			return aux_i;
		}
	}
	return NULL_Q_ITEM;
}