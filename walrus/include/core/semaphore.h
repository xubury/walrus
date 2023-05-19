#pragma once

#include "type.h"

typedef struct Walrus_Semaphore Walrus_Semaphore;

Walrus_Semaphore *walrus_semaphore_create(void);

void walrus_semaphore_destroy(Walrus_Semaphore *semaphore);

bool walrus_semaphore_wait(Walrus_Semaphore *semaphore, i32 ms);

void walrus_semaphore_post(Walrus_Semaphore *semaphore, u32 count);
