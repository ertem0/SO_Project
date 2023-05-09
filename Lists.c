#include "Lists.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <pthread.h>
#include <fcntl.h>  
#include <stdarg.h>
#include <time.h>

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
    if(list->size == 0){
        return NULL;
    }
    for (int i = 0; i < list->size; i++)
    {
        if (strcmp(list->nodes[i].key, key) == 0)
        {
            return &list->nodes[i];
        }
    }
    return NULL;
}

int insert(KeyList *list, char *key, int value)
{
    printf("process %d inserting %s %d\n", getpid(), key, value);
    KeyNode *node = search(list, key);
    printf("process %d node %p\n", getpid(), node);
    if(node != NULL){
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
        printf("new node %s %d %d %d %d %f\n", node->key, node->last, node->min, node->max, node->count, node->mean);
        return 1;
    }
    if (list->size == list->max_size)
    {
        return 0;
    }
    printf("process %d inserting new node\n", getpid());
    KeyNode *new_node = &list->nodes[list->size];
    if(new_node == NULL)
        return 0;
    strcpy(new_node->key, key);
    new_node->last = value;
    new_node->min = value;
    new_node->max = value;
    new_node->count = 1;
    new_node->mean = value;
    list->size++;
    return 2;
}

/*
typedef struct SensorNode {
    char id[MAX_LENGTH];
} SensorNode;

typedef struct SensorList {
    SensorNode *nodes;
    int size;
    int max_size;
} SensorList;
*/

SensorNode *search_sensor(SensorList *list, char *id)
{
    if(list->size == 0){
        return NULL;
    }
    for (int i = 0; i < list->size; i++)
    {
        if (strcmp(list->nodes[i].id, id) == 0)
        {
            return &list->nodes[i];
        }
    }
    return NULL;
}

int insert_sensor(SensorList *list, char *id)
{
    printf("process %d inserting %s\n", getpid(), id);
    SensorNode *node = search_sensor(list, id);
    printf("process %d node %p\n", getpid(), node);
    if (node){
        return 1;
    }
    if (list->size == list->max_size)
    {
        return 0;
    }
    printf("process %d inserting new node\n", getpid());
    SensorNode *new_node = &list->nodes[list->size];
    if(new_node == NULL)
        return 0;
    strcpy(new_node->id, id);
    list->size++;
    return 2;
}