#pragma once

#include <core/type.h>

typedef struct Walrus_Array Walrus_Array;

typedef void (*Walrus_ArrayElementDestroyFunc)(void*);
typedef void (*Walrus_ArrayForeachFunc)(void*, void*);

Walrus_Array* walrus_array_create(u32 element_size, u32 len);
Walrus_Array* walrus_array_create_full(u32 element_size, u32 len, Walrus_ArrayElementDestroyFunc func);

void walrus_array_destroy(Walrus_Array* array);

void walrus_array_clear(Walrus_Array* array);

void walrus_array_fit(Walrus_Array* array);

void walrus_array_resize(Walrus_Array* array, u32 len);

u32 walrus_array_len(Walrus_Array const* array);

void* walrus_array_get(Walrus_Array* array, u32 index);

Walrus_Array* walrus_array_append(Walrus_Array* array, void* data);

Walrus_Array* walrus_array_nappend(Walrus_Array* array, void* data, u32 len);

void walrus_array_foreach(Walrus_Array* array, Walrus_ArrayForeachFunc func, void* userdata);
