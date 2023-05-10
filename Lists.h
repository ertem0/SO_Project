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

typedef struct AlertNode {
    char id[MAX_LENGTH];
    char key[MAX_LENGTH];
    int min, max;
} AlertNode;

typedef struct AlertList {
    AlertNode *nodes;
    int size;
    int max_size;
} AlertList;



KeyNode *search(KeyList *list, char *key);

int insert(KeyList *list, char *key, int value);

int insert_sensor(SensorList *list, char *id);

int insert_alert(AlertList *list, char *id, char *key, int min, int max);

int remove_alert(AlertList *list, char *id);