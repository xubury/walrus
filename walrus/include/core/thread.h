#pragma once

#include "type.h"

typedef struct _Walrus_Thread Walrus_Thread;

typedef i32 (*Walrus_ThreadFn)(Walrus_Thread* self, void* userdata);

Walrus_Thread* walrus_thread_create(void);

void walrus_thread_destroy(Walrus_Thread* thread);

bool walrus_thread_init(Walrus_Thread* thread, Walrus_ThreadFn fn, void* userdata, u32 stack_size);

void walrus_thread_shutdown(Walrus_Thread* thread);
