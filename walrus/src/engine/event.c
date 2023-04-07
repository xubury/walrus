#include <engine/event.h>
#include <core/queue.h>

#include <stdlib.h>
#include <string.h>

static Walrus_Queue *s_event_queue = NULL;

static Walrus_Event *event_alloc(void)
{
    return (Walrus_Event *)malloc(sizeof(Walrus_Event));
}

static void event_free(Walrus_Event *event)
{
    free(event);
}

void walrus_event_init(void)
{
    s_event_queue = walrus_queue_alloc();
}

void walrus_event_shutdown(void)
{
    Walrus_Event *p = NULL;
    // Free unpolled events
    while ((p = walrus_queue_pop(s_event_queue)) != NULL) {
        event_free(p);
    }

    // Free queue itself
    walrus_queue_free(s_event_queue);
}

i32 walrus_event_poll(Walrus_Event *event)
{
    if (event == NULL) return WR_EVENT_INVALID;

    Walrus_Event *p = walrus_queue_pop(s_event_queue);

    if (p) {
        memcpy(event, p, sizeof(Walrus_Event));
        event_free(p);

        return WR_EVENT_SUCCESS;
    }
    else {
        return WR_EVENT_EMPTY;
    }
}

i32 walrus_event_push(Walrus_Event *event)
{
    if (event == NULL) return WR_EVENT_INVALID;

    Walrus_Event *e = event_alloc();

    memcpy(e, event, sizeof(Walrus_Event));
    walrus_queue_push(s_event_queue, e);

    return WR_EVENT_SUCCESS;
}
