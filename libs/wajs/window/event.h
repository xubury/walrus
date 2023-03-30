#pragma once

#include <type.h>

typedef struct {
    u8  device;
    u8  axis;
    f32 x;
    f32 y;
    f32 z;
} AxisEvent;

typedef struct {
    u8   device;
    u16  code;
    bool state;
    u8   mods;
} ButtonEvent;

typedef struct {
    union {
        AxisEvent   axis;
        ButtonEvent button;
    };
} Event;

typedef enum {
    EVENT_SUCCESS = 0,
    EVENT_INVALID = 1,
    EVENT_FULL    = 2,
    EVENT_EMPTY   = 3,
    EVENT_ERROR   = -1,
} EventResult;

i32 poll_event(Event *event);

i32 push_event(Event *event);
