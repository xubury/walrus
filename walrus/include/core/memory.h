#pragma once

#include "type.h"

#include <core/platform.h>

#include <malloc.h>
#if WR_PLATFORM == WR_PLATFORM_POSIX
#include <alloca.h>
#endif

// malloc memory
void* walrus_malloc(u64 size);

// malloc and initialize memory to zeroes.
void* walrus_malloc0(u64 size);

void walrus_free(void* ptr);

// realloc memory
void* walrus_realloc(void* ptr, u64 size);

#define walrus_new(type, size) (type*)walrus_malloc(sizeof(type) * size)

#define walrus_new0(type, size) (type*)walrus_malloc0(sizeof(type) * size)

void* walrus_memdup(void const* ptr, u64 size);

#define walrus_alloca(size) alloca(size);
