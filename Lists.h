#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_LENGTH 32

typedef struct KeyNode {
    char key[MAX_LENGTH];
    int last, min, max, count;
    double mean;
} KeyNode;

typedef struct KeyList {
    KeyNode *nodes;
    int size;
    int max_size;
} KeyList;

typedef struct SensorNode {
    char id[MAX_LENGTH];
} SensorNode;

typedef struct SensorList {
    SensorNode *nodes;
    int size;
    int max_size;
} SensorList;



KeyNode *search(KeyList *list, char *key);

int insert(KeyList *list, char *key, int value);

int insert_sensor(SensorList *list, char *id);