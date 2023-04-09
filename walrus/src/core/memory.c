#include <core/memory.h>

#include <stdlib.h>
#include <string.h>

void* walrus_malloc(u64 size)
{
    void* ptr = malloc(size);
    return ptr;
}

void* walrus_malloc0(u64 size)
{
    void* ptr = walrus_malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

void* walrus_memdup(void const* ptr, u64 size)
{
    void* new_mem = NULL;

    if (ptr && size != 0) {
        new_mem = walrus_malloc(size);
        memcpy(new_mem, ptr, size);
    }
    return new_mem;
}
