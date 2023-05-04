#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABLE_SIZE 307
#define MAX_KEY_LENGTH 32

typedef struct Data {
    int last, min, max, count;
    double mean;
} Data;

typedef struct HashNode {
    char *key;
    Data value;
    struct HashNode *next;
} HashNode;

typedef struct HashTable {
    HashNode **table;
} HashTable;

unsigned int hash(const char *key);

HashTable *create_table();

void insert(HashTable *ht, const char *key, Data value);

Data *search(HashTable *ht, const char *key);

void free_table(HashTable *ht);