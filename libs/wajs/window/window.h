#pragma once

typedef void (*WajsLoopCallback)(void);

void wajs_set_main_loop(WajsLoopCallback callback);
