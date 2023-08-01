#pragma once

#include <engine/editor.h>
#include <engine/system.h>

typedef void* EditorSystem;

POLY_DECLARE_DERIVED(Walrus_System, EditorSystem, editor_system_create)
