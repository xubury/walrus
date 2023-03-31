#include <event.h>
#include <queue.h>

i32 poll_event(Event *event)
{
    if (event == NULL) return EVENT_INVALID;

    /* queue_pop(&event_queue, event); */

    return EVENT_SUCCESS;
}

i32 push_event(Event *event)
{
    if (event == NULL) return EVENT_INVALID;

    /* queue_push(&event_queue, event); */

    return EVENT_SUCCESS;
}
