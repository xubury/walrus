#pragma once

#include <core/list.h>

typedef struct {
    List *head;
    List *tail;
    u64   length;
} Queue;

// Allocate a queue
Queue *queue_alloc(void);

// Free a queue
void queue_free(Queue *queue);

// Push data to the end of the queue
void queue_push(Queue *queue, void *data);

// Pop data from the start of the queue
void *queue_pop(Queue *queue);

// Peek the data from the start of the queue
void *queue_peek(Queue *queue);
