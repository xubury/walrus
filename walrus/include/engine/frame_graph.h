#pragma once

#include <core/type.h>
#include <core/array.h>
#include <core/list.h>
#include <core/hash.h>

typedef struct Walrus_FrameNode Walrus_FrameNode;

typedef struct Walrus_FrameGraph Walrus_FrameGraph;

typedef struct Walrus_FramePipeline Walrus_FramePipeline;

typedef void (*Walrus_FrameNodeCallback)(Walrus_FrameGraph *graph, Walrus_FrameNode const *node);
typedef void (*Walrus_PipelineDestroyCallback)(void *userdata);

struct Walrus_FrameNode {
    char *name;
    u32   index;

    Walrus_FrameNodeCallback func;
};

struct Walrus_FramePipeline {
    char *name;

    Walrus_List *command_list;
    Walrus_List *start;

    Walrus_Array *prevs;
    Walrus_Array *nodes;

    Walrus_PipelineDestroyCallback destroy_func;
    void                          *userdata;
};

struct Walrus_FrameGraph {
    Walrus_HashTable *resources;
    Walrus_HashTable *pipelines;
};

void walrus_fg_init(Walrus_FrameGraph *graph);
void walrus_fg_shutdown(Walrus_FrameGraph *graph);
void walrus_fg_clear(Walrus_FrameGraph *graph);

Walrus_FramePipeline *walrus_fg_add_pipeline(Walrus_FrameGraph *graph, char const *name);
Walrus_FramePipeline *walrus_fg_add_pipeline_full(Walrus_FrameGraph *graph, char const *name,
                                                  Walrus_PipelineDestroyCallback callback, void *uesrdata);
Walrus_FramePipeline *walrus_fg_lookup_pipeline(Walrus_FrameGraph *graph, char const *name);
void                  walrus_fg_connect_pipeline(Walrus_FramePipeline *parent, Walrus_FramePipeline *child);

void walrus_fg_add_node(Walrus_FramePipeline *pipeline, Walrus_FrameNodeCallback func, char const *name);

void  walrus_fg_write(Walrus_FrameGraph *graph, char const *name, u64 handle);
void  walrus_fg_write_ptr(Walrus_FrameGraph *graph, char const *name, void *ptr);
u64   walrus_fg_read(Walrus_FrameGraph const *graph, char const *name);
void *walrus_fg_read_ptr(Walrus_FrameGraph const *graph, char const *name);

void walrus_fg_compile(Walrus_FrameGraph *graph);
void walrus_fg_execute(Walrus_FrameGraph *graph, char const *name);
