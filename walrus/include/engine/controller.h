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
    POLY_TABLE(Walrus_Controller, POLY_INTERFACE(controller_init), POLY_INTERFACE(controller_shutdown),
               POLY_INTERFACE(controller_tick))
};

POLY_PROTOTYPE(void, controller_init, Walrus_Controller *controller)
POLY_PROTOTYPE(void, controller_shutdown, Walrus_Controller *controller)
POLY_PROTOTYPE(void, controller_tick, Walrus_Controller *controller, Walrus_ControllerEvent *event)
