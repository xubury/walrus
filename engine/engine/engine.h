#pragma once

#include <type.h>

typedef void (*EngineRenderCallback)(void);

typedef void (*EngineTickCallback)(f32);

typedef void (*EngineLoopCallback)(EngineRenderCallback, EngineTickCallback);

void engine_run(EngineRenderCallback render, EngineTickCallback tick);
