#include "InternalQueue.h"
InternalQueue *create_queue(int max_size) {
    InternalQueue *queue = (InternalQueue *)malloc(sizeof(InternalQueue));
    queue->head = NULL;
    queue->size = 0;
    queue->max_size = max_size;
    return queue;
}

int add_message(InternalQueue *queue, Payload *payload, int priority) {
    if (queue->size >= queue->max_size) {
        return -1; // Queue is full
    }

    IQMessage *new_msg = (IQMessage *)malloc(sizeof(IQMessage));
    new_msg->payload = payload;
    new_msg->priority = priority;
    new_msg->next = NULL;

    if (queue->head == NULL || queue->head->priority > priority) {
        new_msg->next = queue->head;
        queue->head = new_msg;
    } else {
        IQMessage *current = queue->head;
        while (current->next != NULL && current->next->priority <= priority) {
            current = current->next;
        }
        new_msg->next = current->next;
        current->next = new_msg;
    }

    queue->size++;
    return 0; // IQMessage added successfully
}

Payload *remove_message(InternalQueue *queue) {
    if (queue->head == NULL) {
        return NULL;
    }

    IQMessage *removed_msg = queue->head;
    Payload *payload = removed_msg->payload;
    queue->head = removed_msg->next;
    free(removed_msg);

    queue->size--;
    return payload;
}

void list_messages(InternalQueue *queue) {
    IQMessage *current = queue->head;
    while (current != NULL) {
        printf("Priority: %d, DataType: %d\n", current->priority, current->payload->type);
        current = current->next;
    }
}

void free_queue(InternalQueue *queue) {
    IQMessage *current = queue->head;
    while (current != NULL) {
        IQMessage *temp = current;
        current = current->next;
        free(temp->payload);
        free(temp);
    }
    free(queue);
}

int is_empty(InternalQueue *queue) {
    return queue->size == 0;
}