#include "queue.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX 2048

trs_item createTRS(const char *language, const char *ip, unsigned int port)
{
    trs_item trs = (trs_item) malloc(sizeof(struct trsItem));
    
    trs->port = port;
    if(strlen(language) <= 20 && strlen(ip) <= 20)
    {
        trs->language = (char *) malloc(sizeof(char)*21);
        trs->ip = (char *) malloc(sizeof(char)*21);
        strcpy(trs->ip,ip);
        strcpy(trs->language,language);
    }
    else
        perror("language or ip are too long");
    
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
    node_link node = (node_link) malloc(sizeof(struct q_node ));

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
    trs_list list = (trs_list) malloc(sizeof(struct trsList));

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

void destroylist(trs_list list)
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

char* listLanguages(trs_list list)
{
    node_link aux;
    char* aux_r = (char*) malloc(sizeof(char)*MAX);
    int auxRLen;

    auxRLen = sprintf(aux_r, "%s %d", "ULR", sizeList(list));
    
    for(aux = list->head; aux != NULL; aux = aux->next)
    {
        auxRLen += sprintf(aux_r+auxRLen," %s",aux->item->language);
    }
    *(aux_r+auxRLen) = '\0';
    printf("%s\n",aux_r);
    return aux_r;
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

