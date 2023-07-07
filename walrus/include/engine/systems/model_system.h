#pragma once

#include <flecs.h>
#include <cglm/cglm.h>
#include <engine/model.h>

typedef struct {
    char        *name;
    char        *path;
    Walrus_Model model;
    i32         *ref_count;
} Walrus_ModelRef;

extern ECS_COMPONENT_DECLARE(Walrus_ModelRef);

void walrus_model_system_init(void);

void walrus_model_system_shutdown(void);

void walrus_model_system_load_from_file(char const *name, char const *filename);

bool walrus_model_system_unload(char const *name);

ecs_entity_t walrus_model_instantiate(char const *name, vec3 const trans, versor const rot, vec3 const scale);
