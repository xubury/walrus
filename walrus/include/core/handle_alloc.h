#pragma once

#include "type.h"

#define WR_INVALID_HANDLE 0

typedef u32 Walrus_Handle;

typedef struct {
    Walrus_Handle num_handles;
    Walrus_Handle max_handles;
} Walrus_HandleAlloc;

// Create a handle allocator
Walrus_HandleAlloc *walrus_handle_create(u32 capacity);

// Destroy given allocator
void walrus_handle_destroy(Walrus_HandleAlloc *alloc);

// Allocate a handle from the given handle allocator
Walrus_Handle walrus_handle_alloc(Walrus_HandleAlloc *alloc);

// Free a handle in the given handle allocator
void walrus_handle_free(Walrus_HandleAlloc *alloc, Walrus_Handle handle);

// Validate handle
bool walrus_handle_valid(Walrus_HandleAlloc *alloc, Walrus_Handle handle);
