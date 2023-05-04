#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "UserCommand.h"

typedef enum {
    TYPE_SENSOR_DATA,
    TYPE_USER_COMMAND,
} DataType;

typedef struct SensorData {
    char *id;
    char *key;
    int value;
} SensorData;

union data
{
    SensorData *sensor_data;
    UserCommand *user_command;
};

typedef struct Message {
    DataType type;
    union data *data;
    int priority;
    struct Message *next;
} Message;

typedef struct InternalQueue {
    Message *head;
    int size;
    int max_size;
} InternalQueue;

InternalQueue *create_queue(int max_size);

int add_message(InternalQueue *queue, char *text, int priority);

char *remove_message(InternalQueue *queue);

void list_messages(InternalQueue *queue);

void free_queue(InternalQueue *queue);

int is_empty(InternalQueue *queue);