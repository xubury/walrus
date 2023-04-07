#pragma once

#include <core/list.h>

typedef struct {
    Walrus_List *head;
    Walrus_List *tail;
    u64          length;
} Walrus_Queue;

// Allocate a queue
Walrus_Queue *walrus_queue_alloc(void);

// Free a queue
void walrus_queue_free(Walrus_Queue *queue);

// Push data to the end of the queue
void walrus_queue_push(Walrus_Queue *queue, void *data);

// Pop data from the start of the queue
void *walrus_queue_pop(Walrus_Queue *queue);

// Peek the data from the start of the queue
void *walrus_queue_peek(Walrus_Queue *queue);
