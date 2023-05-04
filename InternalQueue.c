#include "InternalQueue.h"

InternalQueue *create_queue(int max_size) {
    InternalQueue *queue = (InternalQueue *)malloc(sizeof(InternalQueue));
    queue->head = NULL;
    queue->size = 0;
    queue->max_size = max_size;
    return queue;
}

int add_message(InternalQueue *queue, Data *data, int priority) {
    if (queue->size >= queue->max_size) {
        return -1; // Queue is full
    }

    Message *new_msg = (Message *)malloc(sizeof(Message));
    new_msg->data = data;
    new_msg->priority = priority;
    new_msg->next = NULL;

    if (queue->head == NULL || queue->head->priority > priority) {
        new_msg->next = queue->head;
        queue->head = new_msg;
    } else {
        Message *current = queue->head;
        while (current->next != NULL && current->next->priority <= priority) {
            current = current->next;
        }
        new_msg->next = current->next;
        current->next = new_msg;
    }

    queue->size++;
    return 0; // Message added successfully
}

Data *remove_message(InternalQueue *queue) {
    if (queue->head == NULL) {
        return NULL;
    }

    Message *removed_msg = queue->head;
    queue->head = removed_msg->next;
    free(removed_msg);

    queue->size--;
    return removed_msg->data;
}

void list_messages(InternalQueue *queue) {
    Message *current = queue->head;
    while (current != NULL) {
        printf("Priority: %d, Message: %s\n", current->priority, current->type ? "User Command" : "Sensor Data");
        current = current->next;
    }
}

void free_queue(InternalQueue *queue) {
    Message *current = queue->head;
    while (current != NULL) {
        Message *temp = current;
        current = current->next;
        free(temp->data);
        free(temp);
    }
    free(queue);
}

int is_empty(InternalQueue *queue) {
    return queue->size == 0;
}