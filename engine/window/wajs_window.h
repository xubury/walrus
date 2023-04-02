#pragma once

#include <type.h>

typedef void (*WajsRenderCallback)(void);

typedef void (*WajsTickCallback)(f32);

typedef void (*WajsLoopCallback)(WajsRenderCallback, WajsTickCallback);

void wajs_set_main_loop(WajsLoopCallback loop, WajsRenderCallback render, WajsTickCallback tick);
