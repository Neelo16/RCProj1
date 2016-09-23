#include "queue.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

trs_item createTRS(char* language, char* hostname, unsigned int port)
{
    trs_item trs = malloc(sizeof(struct trsItem));
    
    trs->hostname = hostname;
    trs->port;
    if(strlen(language) <= 20)
        trs->language = language;
    else
        perror("language is too long");
    
    return trs;
}

void destroyTRS(trs_item trs)
{
    free(trs);
}

node_link createNode(trs_item trs)
{
    node_link node = malloc(sizeof(struct q_node ));

    node->item = trs;
    node->next = NULL;

    return node;
}

void destroyNode(node_link node)
{
    free(node->item);
    free(node);
}

trs_list createList()
{
    trs_list list = malloc(sizeof(struct trsList));

    list->head = NULL;
    list->size = 0;

    return list;
}

void addTRS(trs_list list, trs_item trs)
{
    node_link aux;
    node_link node = createNode(trs);

    if(list->head == NULL)
    {
        list->head = node;
    }else
    {
        aux = list->head;
        list->head = node;
        node->next = aux;
    }

    list->size++;
}

void removeTRS(trs_list list, char* language)
{
    node_link aux, aux2;
    trs_item trs_aux;
    
    if(list->head->item->language == language)
    {
        aux = list->head;
        list->head = aux->next;
        destroyNode(aux);
        list->size--;
        return;
    }
    for(aux2 = list->head; aux2 != NULL; aux2 = aux)
    {
        aux = aux2->next;
        if(aux->item->language == language)
        {

            aux2->next = aux->next;
            destroyNode(aux);
            list->size--;
            break;
        }
        
    }
    if(aux2 == NULL)
    {
        perror("language not found");
    }   
}

void destroyList(trs_list list)
{
    node_link aux;
    aux = list->head;

    while(list->head != NULL){
        list->head = aux->next;
        destroyNode(aux);
    }
}

int sizeList(trs_list list)
{
    return list->size;
}

void showList(trs_list list)
{
    node_link aux;
    
    printf("List ->");
    for(aux = list->head; aux != NULL; aux = aux->next)
    {
        printf(" %s ->", aux->item->language);
    }
    printf("\n");
}
