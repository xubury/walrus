#pragma once

#include <flecs.h>
#include <cglm/cglm.h>
#include <engine/model.h>

extern ECS_COMPONENT_DECLARE(Walrus_Model);

void walrus_model_system_init(void);

void walrus_model_system_shutdown(void);

void walrus_model_system_load_from_file(char const *name, char const *filename);

bool walrus_model_system_unload(char const *name);

ecs_entity_t walrus_model_instantiate(char const *name, vec3 const trans, versor const rot, vec3 const scale);
