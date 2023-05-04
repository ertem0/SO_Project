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

    Message *new_msg = (Message *)malloc(sizeof(Message));
    new_msg->payload = payload;
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

Payload *remove_message(InternalQueue *queue) {
    if (queue->head == NULL) {
        return NULL;
    }

    Message *removed_msg = queue->head;
    Payload *payload = removed_msg->payload;
    queue->head = removed_msg->next;
    free(removed_msg);

    queue->size--;
    return removed_msg->payload;
}

void list_messages(InternalQueue *queue) {
    Message *current = queue->head;
    while (current != NULL) {
        printf("Priority: %d, Message: %s\n", current->priority, current->payload->type ? "User Command" : "Sensor Data");
        current = current->next;
    }
}

void free_queue(InternalQueue *queue) {
    Message *current = queue->head;
    while (current != NULL) {
        Message *temp = current;
        current = current->next;
        if(temp->payload->type == TYPE_SENSOR_DATA) {
            free(temp->payload->data->sensor_data->id);
            free(temp->payload->data->sensor_data->key);
            free(temp->payload->data);
        } else {
            free(temp->payload->data->user_command->command);
            for(int i = 0; i < temp->payload->data->user_command->num_args; i++){
                free(temp->payload->data->user_command->args[i].argchar);
                }
            free(temp->payload->data);
        }
        free(temp->payload->data);
        free(temp->payload);
        free(temp);
    }
    free(queue);
}

int is_empty(InternalQueue *queue) {
    return queue->size == 0;
}