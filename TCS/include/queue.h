#ifndef __LIST__
#define __LIST__

typedef struct q_node* node_link;
typedef struct trsItem* trs_item;
typedef struct trsList* trs_list;

struct trsItem
{
    char* language;
    char* hostname;
    unsigned int port;
};

struct q_node
{
    trs_item item;
    node_link next;
};

struct trsList
{
    node_link head;
    int size;

};


trs_item createTRS(char* language, char* hostname, unsigned int port);
void destroyTRS(trs_item trs);
node_link createNode(trs_item trs);
void destroyNode(node_link node);
trs_list createList();
void addTRS(trs_list list, trs_item trs);
void removeTRS(trs_list list, char* language);
int sizeList(trs_list list);
void destroyList(trs_list list);
void showList(trs_list list);

#endif
