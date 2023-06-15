#include <core/array.h>
#include <core/memory.h>
#include <core/macro.h>
#include <core/math.h>
#include <core/log.h>

#include <string.h>

#define MIN_ARRAY_SIZE 16

struct Walrus_Array {
    u32 capcacity;
    u32 len;
    u32 element_size;

    u8* data;
};

static void array_maybe_expand(Walrus_Array* array, u32 inc)
{
    u32 max_cap = walrus_min(UINT64_MAX / 2 / array->element_size, UINT32_MAX);
    if (walrus_unlikely(max_cap - array->len < inc)) {
        // overflow
        walrus_error("array size overflow");
    }

    u32 dst_len = array->len + inc;
    if (dst_len > array->capcacity) {
        // resize
        u64 dst_size     = walrus_nearest_pow(dst_len * array->element_size);
        dst_size         = walrus_max(dst_size, MIN_ARRAY_SIZE);
        array->data      = walrus_realloc(array->data, dst_size);
        array->capcacity = walrus_min(dst_size / array->element_size, UINT32_MAX);
    }
}

Walrus_Array* walrus_array_create(u32 element_size, u32 len)
{
    Walrus_Array* array = walrus_new(Walrus_Array, 1);
    array->element_size = element_size;
    array->capcacity    = 0;
    array->len          = 0;
    array->data         = NULL;

    walrus_array_resize(array, len);

    return array;
}

void walrus_array_destroy(Walrus_Array* array)
{
    walrus_free(array->data);
    walrus_free(array);
}

void walrus_array_clear(Walrus_Array* array)
{
    array->len = 0;
}

void walrus_array_fit(Walrus_Array* array)
{
    array->capcacity = array->len;
    array->data      = walrus_realloc(array->data, array->capcacity * array->element_size);
}

void walrus_array_resize(Walrus_Array* array, u32 len)
{
    array_maybe_expand(array, walrus_max(array->len, len) - array->len);
    array->len = len;
}

u32 walrus_array_len(Walrus_Array const* array)
{
    return array->len;
}

void* walrus_array_get(Walrus_Array* array, u32 index)
{
    if (index >= array->len) {
        return NULL;
    }
    return array->data + array->element_size * index;
}

Walrus_Array* walrus_array_append(Walrus_Array* array, void* data)
{
    return walrus_array_nappend(array, data, 1);
}

Walrus_Array* walrus_array_nappend(Walrus_Array* array, void* data, u32 len)
{
    array_maybe_expand(array, len);
    if (data != NULL) {
        memcpy(array->data + array->element_size * array->len, data, len * array->element_size);
    }
    array->len += len;

    return array;
}
