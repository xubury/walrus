#pragma once

#include <list.h>

typedef struct {
    List *head;
    List *tail;
    u8    length;
} Queue;

// Allocate a queue
Queue *queue_alloc(void);

// Free a queue
void queue_free(Queue *queue);

void queue_push(Queue *queue, void *data);

void *queue_pop(Queue *queue);

void *queue_peek(Queue *queue);
