#pragma once

#include "type.h"

// void walrus_radix_sort(u32* keys, u32* tempKeys, u32 size);

void walrus_radix_sort(u32* keys, u32* temp_keys, void* values, void* temp_values, u32 size, u32 element_size);

void walrus_radix_sort64(u64* keys, u64* temp_keys, void* values, void* temp_values, u32 size, u32 element_size);
