#include <core/queue.h>
#include <stdlib.h>

Queue *queue_alloc(void)
{
    Queue *new  = malloc(sizeof(Queue));
    new->head   = NULL;
    new->tail   = NULL;
    new->length = 0;

    return new;
}

void queue_free(Queue *queue)
{
    list_free(queue->head);
    free(queue);
}

void queue_push(Queue *queue, void *data)
{
    queue->tail = list_append(queue->tail, data);
    if (queue->tail->next) {
        queue->tail = queue->tail->next;
    }
    else {
        queue->head = queue->tail;
    }
    ++queue->length;
}

void *queue_pop(Queue *queue)
{
    if (queue->head) {
        List *node = queue->head;
        void *data = node->data;

        queue->head = node->next;
        if (queue->head) {
            queue->head->prev = NULL;
        }
        else {
            queue->tail = NULL;
        }

        list_free1(node);

        --queue->length;

        return data;
    }
    return NULL;
}

void *queue_peek(Queue *queue)
{
    if (queue->tail) {
        return queue->tail->data;
    }
    return NULL;
}
