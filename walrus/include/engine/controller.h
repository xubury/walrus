#pragma once

#include <core/transform.h>
#include <core/type.h>
#include <engine/input_map.h>

#include <flecs.h>

typedef struct Walrus_Controller      Walrus_Controller;
typedef struct Walrus_ControllerEvent Walrus_ControllerEvent;

typedef void (*Walrus_ControllerTickCallback)(Walrus_ControllerEvent *event);
typedef void (*Walrus_ControllerInitCallback)(Walrus_Controller *controller);
typedef void (*Walrus_ControllerShutdownCallback)(Walrus_Controller *controller);

struct Walrus_ControllerEvent {
    ecs_entity_t      entity;
    Walrus_Transform *transform;
    f32               delta_time;
    void             *userdata;
};

struct Walrus_Controller {
    Walrus_ControllerInitCallback     init;
    Walrus_ControllerShutdownCallback shutdown;
    Walrus_ControllerTickCallback     tick;

    void *userdata;

    Walrus_InputMap map;
};
