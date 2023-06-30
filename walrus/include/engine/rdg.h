#pragma once

#include <core/type.h>
#include <core/array.h>
#include <core/hash.h>
#include <rhi/rhi.h>

/* render dependency graph */

typedef struct Walrus_RenderNode Walrus_RenderNode;

typedef void (*Walrus_RenderNodeCallback)(Walrus_RenderNode const *node, void *userdata);

struct Walrus_RenderNode {
    char *name;
    u32   index;

    Walrus_RenderNodeCallback func;
};

typedef struct {
    Walrus_HashTable *resources;
    Walrus_Array     *nodes;
} Walrus_FrameGraph;

void walrus_fg_init(Walrus_FrameGraph *graph);
void walrus_fg_shutdown(Walrus_FrameGraph *graph);

void walrus_fg_add_node(Walrus_FrameGraph *graph, Walrus_RenderNodeCallback func, char const *name);
void walrus_fg_clear(Walrus_FrameGraph *graph);

void walrus_fg_write(Walrus_FrameGraph *graph, char const *name, u64 handle);
u64  walrus_fg_read(Walrus_FrameGraph *graph, char const *name);

void walrus_fg_compile(Walrus_FrameGraph *graph);
void walrus_fg_execute(Walrus_FrameGraph *graph, void *userdata);
