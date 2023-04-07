#pragma once

#include <core/type.h>

typedef enum {
    WR_INPUT_NONE = 0,

    WR_INPUT_MOUSE,
    WR_INPUT_KEYBOARD,
    WR_INPUT_JOYSTICK,
} Walrus_InputType;

typedef struct {
    u8  device;
    u8  axis;
    i32 x;
    i32 y;
    i32 z;
    u8  mods;
} Walrus_AxisEvent;

typedef struct {
    u8   device;
    u16  button;
    bool state;
    u8   mods;
} Walrus_ButtonEvent;

typedef struct {
    u32 width;
    u32 height;
} Walrus_ResolutionEvent;

typedef enum {
    WR_EVENT_TYPE_AXIS,
    WR_EVENT_TYPE_BUTTON,
    WR_EVENT_TYPE_RESOLUTION,
    WR_EVENT_TYPE_EXIT,
} Walrus_EventType;

typedef struct {
    Walrus_EventType type;
    union {
        Walrus_AxisEvent       axis;
        Walrus_ButtonEvent     button;
        Walrus_ResolutionEvent resolution;
    };
} Walrus_Event;

typedef enum {
    WR_EVENT_SUCCESS = 0,
    WR_EVENT_INVALID = 1,
    WR_EVENT_FULL    = 2,
    WR_EVENT_EMPTY   = 3,
    WR_EVENT_ERROR   = -1,
} Walrus_EventResult;

void walrus_event_init(void);

void walrus_event_shutdown(void);

i32 walrus_event_poll(Walrus_Event *event);

i32 walrus_event_push(Walrus_Event *event);
