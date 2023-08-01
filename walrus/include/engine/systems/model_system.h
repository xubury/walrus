#pragma once

#include <flecs.h>
#include <cglm/cglm.h>
#include <engine/model.h>
#include <engine/system.h>

typedef struct {
    char        *name;
    char        *path;
    Walrus_Model model;
    i32         *ref_count;
} Walrus_ModelRef;

typedef struct {
    Walrus_HashTable *table;
} ModelSystem;

POLY_DECLARE_DERIVED(Walrus_System, ModelSystem, model_system_create)

extern ECS_COMPONENT_DECLARE(Walrus_ModelRef);

void walrus_model_system_load_from_file(Walrus_System *sys, char const *name, char const *filename);

bool walrus_model_system_unload(Walrus_System *sys, char const *name);

ecs_entity_t walrus_model_instantiate(Walrus_System *sys, char const *name, vec3 const trans, versor const rot,
                                      vec3 const scale);
