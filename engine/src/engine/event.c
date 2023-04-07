#include <engine/event.h>
#include <core/queue.h>

#include <stdlib.h>
#include <string.h>

static Queue *s_event_queue = NULL;

static Event *event_alloc(void)
{
    return (Event *)malloc(sizeof(Event));
}

static void event_free(Event *event)
{
    free(event);
}

void event_init(void)
{
    s_event_queue = queue_alloc();
}

void event_shutdown(void)
{
    Event *p = NULL;
    // Free unpolled events
    while ((p = queue_pop(s_event_queue)) != NULL) {
        event_free(p);
    }

    // Free queue itself
    queue_free(s_event_queue);
}

i32 event_poll(Event *event)
{
    if (event == NULL) return EVENT_INVALID;

    Event *p = queue_pop(s_event_queue);

    if (p) {
        memcpy(event, p, sizeof(Event));
        event_free(p);

        return EVENT_SUCCESS;
    }
    else {
        return EVENT_EMPTY;
    }
}

i32 event_push(Event *event)
{
    if (event == NULL) return EVENT_INVALID;

    Event *e = event_alloc();

    memcpy(e, event, sizeof(Event));
    queue_push(s_event_queue, e);

    return EVENT_SUCCESS;
}
