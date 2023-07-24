#include <core/handle_alloc.h>
#include <core/memory.h>

struct Walrus_HandleAlloc {
    Walrus_Handle num_handles;
    Walrus_Handle max_handles;
};

static Walrus_Handle *get_dense_ptr(Walrus_HandleAlloc *alloc)
{
    return (Walrus_Handle *)((u8 *)alloc + sizeof(Walrus_HandleAlloc));
}

static Walrus_Handle *get_sparse_ptr(Walrus_HandleAlloc *alloc)
{
    return get_dense_ptr(alloc) + alloc->max_handles;
}

static void reset(Walrus_HandleAlloc *alloc)
{
    alloc->num_handles    = 0;
    Walrus_Handle *dense  = get_dense_ptr(alloc);
    Walrus_Handle *sparse = get_sparse_ptr(alloc);
    for (Walrus_Handle i = 0; i < alloc->max_handles; ++i) {
        dense[i]  = i + 1;
        sparse[i] = alloc->max_handles;
    }
}

Walrus_HandleAlloc *walrus_handle_create(u32 capacity)
{
    Walrus_HandleAlloc *alloc = walrus_malloc(sizeof(Walrus_HandleAlloc) + capacity * 2 * sizeof(Walrus_Handle));

    if (alloc) {
        alloc->max_handles = capacity;
        reset(alloc);
    }
    return alloc;
}

void walrus_handle_destroy(Walrus_HandleAlloc *alloc)
{
    walrus_free(alloc);
}

Walrus_Handle walrus_handle_alloc(Walrus_HandleAlloc *alloc)
{
    if (alloc->num_handles < alloc->max_handles) {
        Walrus_Handle index = alloc->num_handles;
        ++alloc->num_handles;

        Walrus_Handle *dense  = get_dense_ptr(alloc);
        Walrus_Handle *sparse = get_sparse_ptr(alloc);
        Walrus_Handle  handle = dense[index];
        sparse[index]         = handle;

        return handle;
    }

    return WR_INVALID_HANDLE;
}

void walrus_handle_free(Walrus_HandleAlloc *alloc, Walrus_Handle handle)
{
    Walrus_Handle *dense  = get_dense_ptr(alloc);
    Walrus_Handle *sparse = get_sparse_ptr(alloc);
    Walrus_Handle  index  = sparse[handle];
    --alloc->num_handles;
    Walrus_Handle temp        = dense[alloc->num_handles];
    dense[alloc->num_handles] = handle;
    sparse[temp]              = index;
    dense[index]              = temp;
}

bool walrus_handle_valid(Walrus_HandleAlloc *alloc, Walrus_Handle handle)
{
    Walrus_Handle *dense  = get_dense_ptr(alloc);
    Walrus_Handle *sparse = get_sparse_ptr(alloc);
    Walrus_Handle  index  = sparse[handle];

    return index < alloc->num_handles && dense[index] == handle;
}
