#ifndef __LIST__
#define __LIST__

typedef struct q_node* node_link;
typedef struct trsItem* trs_item;
typedef struct trsList* trs_list;

struct trsItem
{
    char language[21];
    char ip[21];
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


trs_item createTRS(const char* language, const char* ip, unsigned int port);
char* getLanguage(trs_item trs);
char* getIp(trs_item trs);
int getPort(trs_item trs);
void destroyTRS(trs_item trs);
node_link createNode(trs_item trs);
void destroyNode(node_link node);
trs_list createList();
void addTRSItem(trs_list list, trs_item trs);
void removeTRS(trs_list list, char* language);
int sizeList(trs_list list);
void destroyList(trs_list list);
void showList(trs_list list);
void listLanguages(trs_list list, char *aux_r);
trs_item findTRS(trs_list list, char* language);

#endif
