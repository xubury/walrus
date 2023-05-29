#pragma once

#include <core/type.h>

typedef struct Walrus_Array Walrus_Array;

Walrus_Array* walrus_array_create(u32 element_size, u32 len);

#define walrus_array_create_full(type, len, data) \
    (walrus_array_nappend(walrus_array_create(sizeof(type), len), data, len))

void walrus_array_destroy(Walrus_Array* array);

void walrus_array_clear(Walrus_Array* array);

void walrus_array_fit(Walrus_Array* array);

void walrus_array_resize(Walrus_Array* array, u32 len);

u32 walrus_array_len(Walrus_Array const* array);

void* walrus_array_get(Walrus_Array* array, u32 index);

#define walrus_array_get_ptr(array, type, index) ((type*)walrus_array_get(array, index))
#define walrus_array_get_val(array, type, index) (*walrus_array_get_ptr(array, type, index))

Walrus_Array* walrus_array_append(Walrus_Array* array, void* data);

Walrus_Array* walrus_array_nappend(Walrus_Array* array, void* data, u32 len);
