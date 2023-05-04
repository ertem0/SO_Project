#include "HashTable.h"

unsigned int hash(const char *key) {
    unsigned int hash_value = 0;
    while (*key) {
        hash_value = (hash_value * 31) + *key++;
    }
    return hash_value % TABLE_SIZE;
}

HashTable *create_table() {
    HashTable *ht = malloc(sizeof(HashTable));
    ht->table = malloc(sizeof(HashNode *) * TABLE_SIZE);
    for (int i = 0; i < TABLE_SIZE; i++) {
        ht->table[i] = NULL;
    }
    return ht;
}

void insert(HashTable *ht, const char *key, Data value) {
    unsigned int index = hash(key);
    HashNode *node = malloc(sizeof(HashNode));
    node->key = strdup(key);
    node->value = value;
    node->next = ht->table[index];
    ht->table[index] = node;
}

Data *search(HashTable *ht, const char *key) {
    unsigned int index = hash(key);
    HashNode *node = ht->table[index];
    while (node) {
        if (strcmp(node->key, key) == 0) {
            return &node->value;
        }
        node = node->next;
    }
    return NULL;
}

void free_table(HashTable *ht) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        HashNode *node = ht->table[i];
        while (node) {
            HashNode *temp = node;
            node = node->next;
            free(temp->key);
            free(temp);
        }
    }
    free(ht->table);
    free(ht);
}
