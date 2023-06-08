#pragma once

#include "type.h"

typedef i32 (*ComparisonFn)(void const* lhs, void const* rhs);

void walrus_quick_sort(void* data, u32 num, u32 stride, ComparisonFn fn);

void walrus_radix_sort(u32* keys, u32* temp_keys, void* values, void* temp_values, u32 size, u32 element_size);

void walrus_radix_sort64(u64* keys, u64* temp_keys, void* values, void* temp_values, u32 size, u32 element_size);
