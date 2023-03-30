#include <event.h>

i32 poll_event(Event *event)
{
    if (event == NULL) return EVENT_INVALID;

    return EVENT_SUCCESS;
}

i32 push_event(Event *event)
{
    if (event == NULL) return EVENT_INVALID;

    return EVENT_SUCCESS;
}
