#pragma once

#include <core/type.h>
#include <core/cpoly.h>

#include <flecs.h>

typedef struct {
    char         name[256];
    ecs_world_t *ecs;
    POLY_TABLE(Walrus_System, POLY_INTERFACE(on_system_init), POLY_INTERFACE(on_system_shutdown),
               POLY_INTERFACE(on_system_render))
} Walrus_System;

POLY_PROTOTYPE(void, on_system_init, Walrus_System *)
POLY_PROTOTYPE(void, on_system_shutdown, Walrus_System *)
POLY_PROTOTYPE(void, on_system_render, Walrus_System *)
