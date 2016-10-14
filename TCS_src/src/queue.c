#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "util.h"
#include "queue.h"

#define MAX 2048

trs_item createTRS(const char *language, const char *ip, unsigned int port)
{
    trs_item trs = (trs_item) safeMalloc(sizeof(struct trsItem));
    
    trs->port = port;
    if(strlen(language) <= 20 && strlen(ip) <= 20)
    {
        strcpy(trs->ip,ip);
        strcpy(trs->language,language);
    }
    else
        fprintf( stderr, "language or ip are too long");
    
    return trs;
}

int getPort(trs_item trs)
{
    return trs->port;
}

char* getLanguage(trs_item trs)
{
    return trs->language;
} 

char* getIp(trs_item trs)
{
    return trs->ip;
}

void destroyTRS(trs_item trs)
{
    free(trs);
}

node_link createNode(trs_item trs)
{
    node_link node = (node_link) safeMalloc(sizeof(struct q_node ));

    node->item = trs;
    node->next = NULL;

    return node;
}

void destroyNode(node_link node)
{
    destroyTRS(node->item);
    free(node);
}

trs_list createList()
{
    trs_list list = (trs_list) safeMalloc(sizeof(struct trsList));

    list->head = NULL;
    list->size = 0;

    return list;
}
void addTRSItem(trs_list list, trs_item trs)
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
    
    if(!strcmp( list->head->item->language, language))
    {
        aux = list->head;
        list->head = aux->next;
        destroyNode(aux);
        list->size--;
        return;
    }
    for(aux2 = list->head; aux2->next != NULL; aux2 = aux)
    {
        aux = aux2->next;
        if(!strcmp(aux->item->language, language))
        {
            aux2->next = aux->next;
            destroyNode(aux);
            list->size--;
            break;
        }
        
    }
    if(aux2 == NULL)
        fprintf(stderr, "language not found");
}

trs_item findTRS(trs_list list, char* language)
{
    node_link aux;

    for(aux = list->head; aux != NULL; aux = aux->next)
    {
        if(!strcmp(aux->item->language, language))
            return aux->item;
    }

    return NULL;
}

void destroyList(trs_list list)
{
    node_link aux;
    
    aux = list->head;
    
    while(aux != NULL)
    {
        list->head = list->head->next;
        destroyNode(aux);
        aux = list->head;
    }

    free(list);    
}

int sizeList(trs_list list)
{
    return list->size;
}

void listLanguages(trs_list list, char *aux_r)
{
    node_link aux;
    int auxRLen;

    auxRLen = sprintf(aux_r, "%s %d", "ULR", sizeList(list));
    for(aux = list->head; aux != NULL; aux = aux->next)
    {
        printf(" %s",aux->item->language);
        auxRLen += sprintf(aux_r+auxRLen," %s",aux->item->language);
    }
    puts("");

    *(aux_r+auxRLen) = '\n';
    *(aux_r+auxRLen+1) = '\0';
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
