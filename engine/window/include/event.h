#pragma once

#include <type.h>

typedef enum {
    INPUT_MOUSE,
    INPUT_KEYBOARD,
    INPUT_JOYSTICK,
} InputDeivceType;

typedef struct {
    u8  device;
    u8  axis;
    i32 x;
    i32 y;
    i32 z;
    u8  mods;
} AxisEvent;

typedef struct {
    u8   device;
    u16  code;
    bool state;
    u8   mods;
} ButtonEvent;

typedef enum {
    EVENT_TYPE_AXIS,
    EVENT_TYPE_BUTTON
} EventType;

typedef struct {
    EventType type;
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

void event_init(void);

void event_destroy(void);

i32 event_poll(Event *event);

i32 event_push(Event *event);
