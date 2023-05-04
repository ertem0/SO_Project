#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_KEY_LENGTH 32

typedef struct KeyNode {
    char Key[MAX_KEY_LENGTH];
    int last, min, max, count;
    double mean;
} KeyNode;

typedef struct KeyList {
    KeyNode *nodes;
    int size;
    int max_size;
} KeyList;