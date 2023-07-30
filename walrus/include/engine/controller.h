#pragma once

#include <core/transform.h>
#include <core/cpoly.h>
#include <engine/input_map.h>

#include <flecs.h>

typedef struct Walrus_Controller      Walrus_Controller;
typedef struct Walrus_ControllerEvent Walrus_ControllerEvent;

struct Walrus_ControllerEvent {
    ecs_entity_t      entity;
    Walrus_Transform *transform;
    f32               delta_time;
};

struct Walrus_Controller {
    Walrus_InputMap map;
    POLY_TABLE(Walrus_Controller, POLY_INTERFACE(init), POLY_INTERFACE(shutdown), POLY_INTERFACE(tick))
};

POLY_PROTOTYPE(void, shutdown, Walrus_Controller *controller)
POLY_PROTOTYPE(void, init, Walrus_Controller *controller)
POLY_PROTOTYPE(void, tick, Walrus_Controller *controller, Walrus_ControllerEvent *event)
