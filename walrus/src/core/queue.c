#include <core/queue.h>
#include <stdlib.h>

Walrus_Queue *walrus_queue_alloc(void)
{
    Walrus_Queue *new  = malloc(sizeof(Walrus_Queue));
    new->head   = NULL;
    new->tail   = NULL;
    new->length = 0;

    return new;
}

void walrus_queue_free(Walrus_Queue *queue)
{
    walrus_list_free(queue->head);
    free(queue);
}

void walrus_queue_push(Walrus_Queue *queue, void *data)
{
    queue->tail = walrus_list_append(queue->tail, data);
    if (queue->tail->next) {
        queue->tail = queue->tail->next;
    }
    else {
        queue->head = queue->tail;
    }
    ++queue->length;
}

void *walrus_queue_pop(Walrus_Queue *queue)
{
    if (queue->head) {
        Walrus_List *node = queue->head;
        void *data = node->data;

        queue->head = node->next;
        if (queue->head) {
            queue->head->prev = NULL;
        }
        else {
            queue->tail = NULL;
        }

        walrus_list_free1(node);

        --queue->length;

        return data;
    }
    return NULL;
}

void *walrus_queue_peek(Walrus_Queue *queue)
{
    if (queue->tail) {
        return queue->tail->data;
    }
    return NULL;
}
