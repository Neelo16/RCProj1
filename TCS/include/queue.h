#ifndef QUEUE
#define QUEUE 

#include <stdio.h>
#include <stdlib.h>
#include "q_item.h"

typedef struct q_node* q_link;

struct q_node { 
        Q_item item; 
        q_link next; 
};  								

typedef struct queue { 
     q_link head; 						
     q_link tail; 					
     int size; 						  

} *Queue; 

/*
 * Estrutura FIFO (embora seja possivel remover qualquer elemento).
 * A destruicao dos items retirados da Queue e da responsabilidade
 * do utilizador, e estes nao sao apagados quando a Queue e apagada.
 * 
 */
									


q_link create_q_node(Q_item i);
void destroy_q_node(q_link n);
Queue createQueue();
void q_push(Queue q, Q_item i);
Q_item q_pop(Queue q); 
Q_item q_find(Queue q, Q_KEY k); 
Q_item q_remove(Queue q, Q_KEY k); 
void q_show(Queue q);
void deleteQueue(Queue q);


#endif