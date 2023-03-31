#pragma once

#include <type.h>

typedef struct {
    u32 head;
    u32 tail;
    u32 element_size;
    u32 size;
    u32 capacity;
    u8 *data;
} Queue;

Queue *queue_new(i32 element_size, i32 capacity);

void queue_free(Queue *queue);

bool queue_empty(Queue *queue);

bool queue_push(Queue *queue, const void *data);

bool queue_pop(Queue *queue, void *data);
