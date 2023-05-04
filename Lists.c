#include "Lists.h"

#define MAX_KEY_LENGTH 32

// typedef struct KeyNode {
//     char Key[MAX_KEY_LENGTH];
//     int last, min, max, count;
//     double mean;
// } KeyNode;

// typedef struct KeyList {
//     KeyNode *nodes;
//     int size;
//     int max_size;
// } KeyList;

KeyNode *search(KeyList *list, char *key)
{
    for (int i = 0; i < list->size; i++)
    {
        if (strcmp(list->nodes[i].Key, key) == 0)
        {
            return &list->nodes[i];
        }
    }
    return NULL;
}

int insert(KeyList *list, char *key, int value)
{
    KeyNode *node = search(list, key);
    if (node){
        node->last = value;
        node->count++;
        node->mean = (node->mean * (node->count - 1) + value) / node->count;
        if (value < node->min)
        {
            node->min = value;
        }
        if (value > node->max)
        {
            node->max = value;
        }
        return 1;
    }
    if (list->size == list->max_size)
    {
        return 0;
    }
    KeyNode *new_node = &list->nodes[list->size];
    strcpy(new_node->Key, key);
    new_node->last = value;
    new_node->min = value;
    new_node->max = value;
    new_node->count = 1;
    new_node->mean = value;
    list->size++;
    return 1;
}
