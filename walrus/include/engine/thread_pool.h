#pragma once

#include <core/type.h>

#include <core/semaphore.h>

typedef void (*ThreadTaskFn)(void *userdata);

void walrus_thread_pool_init(u8 num_threads);

void walrus_thread_pool_shutdown(void);

void walrus_thread_pool_queue(ThreadTaskFn func, void *userdata);
