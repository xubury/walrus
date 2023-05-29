#pragma once

#include <core/type.h>

#include <core/semaphore.h>

typedef i32 (*ThreadTaskFn)(void *userdata);

typedef struct {
    // internal use only, to get exitcode use `walrus_thread_pool_result_get`
    Walrus_Semaphore *sem;
    i32               exit_code;
} Walrus_ThreadResult;

void walrus_thread_pool_init(u8 num_threads);

void walrus_thread_pool_shutdown(void);

void walrus_thread_pool_queue(ThreadTaskFn func, void *userdata, Walrus_ThreadResult *res);

i32 walrus_thread_pool_result_get(Walrus_ThreadResult *res, i32 ms);
