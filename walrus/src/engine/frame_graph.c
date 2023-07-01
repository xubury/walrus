#include <engine/frame_graph.h>
#include <core/memory.h>
#include <core/macro.h>
#include <core/string.h>
#include <core/log.h>
#include <core/assert.h>

void node_free(void *ptr)
{
    Walrus_FrameNode *node = ptr;
    walrus_str_free(node->name);
}

void pipeline_free(void *ptr)
{
    Walrus_FramePipeline *pipeline = ptr;
    walrus_str_free(pipeline->name);
    walrus_list_free(pipeline->command_list);
    walrus_array_destroy(pipeline->nodes);
    walrus_free(pipeline);
}

void walrus_fg_init(Walrus_FrameGraph *graph)
{
    graph->resources =
        walrus_hash_table_create_full(walrus_str_hash, walrus_str_equal, (Walrus_KeyDestroyFunc)walrus_str_free, NULL);
    graph->pipelines = walrus_hash_table_create_full(walrus_str_hash, walrus_str_equal, NULL, pipeline_free);
}

void walrus_fg_shutdown(Walrus_FrameGraph *graph)
{
    walrus_hash_table_destroy(graph->resources);
    walrus_hash_table_destroy(graph->pipelines);
}

Walrus_FramePipeline *walrus_fg_add_pipeline(Walrus_FrameGraph *graph, char const *name)
{
    Walrus_FramePipeline *pipeline = walrus_malloc(sizeof(Walrus_FramePipeline));
    pipeline->name                 = walrus_str_dup(name);
    pipeline->prev                 = NULL;
    pipeline->command_list         = walrus_list_alloc();
    pipeline->nodes                = walrus_array_create_full(sizeof(Walrus_FrameNode), 0, node_free);

    walrus_hash_table_insert(graph->pipelines, pipeline->name, pipeline);
    return pipeline;
}

Walrus_FramePipeline *walrus_fg_lookup_pipeline(Walrus_FrameGraph *graph, char const *name)
{
    if (walrus_hash_table_contains(graph->pipelines, name)) {
        return walrus_hash_table_lookup(graph->pipelines, name);
    }
    else {
        return NULL;
    }
}

void walrus_fg_connect_pipeline(Walrus_FramePipeline *parent, Walrus_FramePipeline *child)
{
    child->prev = parent;
}

void walrus_fg_add_node(Walrus_FramePipeline *pipeline, Walrus_FrameNodeCallback func, char const *name)
{
    Walrus_FrameNode node = {.name = walrus_str_dup(name), .index = walrus_array_len(pipeline->nodes), .func = func};

    walrus_array_append(pipeline->nodes, &node);
}

void walrus_fg_clear(Walrus_FrameGraph *graph)
{
    walrus_hash_table_remove_all(graph->pipelines);
    walrus_hash_table_remove_all(graph->resources);
}

void walrus_fg_write(Walrus_FrameGraph *graph, char const *name, u64 handle)
{
    walrus_hash_table_insert(graph->resources, walrus_str_dup(name), walrus_val_to_ptr(handle));
}

void walrus_fg_write_ptr(Walrus_FrameGraph *graph, char const *name, void *ptr)
{
    walrus_hash_table_insert(graph->resources, walrus_str_dup(name), ptr);
}

u64 walrus_fg_read(Walrus_FrameGraph const *graph, char const *name)
{
    return walrus_ptr_to_val(walrus_hash_table_lookup(graph->resources, name));
}

void *walrus_fg_read_ptr(Walrus_FrameGraph const *graph, char const *name)
{
    return walrus_hash_table_lookup(graph->resources, name);
}

void construct_pipeline_command(void const *key, void *value, void *userdata)
{
    walrus_unused(key);
    walrus_unused(userdata);
    Walrus_FramePipeline *target = value;
    Walrus_FramePipeline *p      = target;

    walrus_list_free(target->command_list);
    target->command_list = walrus_list_append(NULL, target);

    while (p->prev != NULL) {
        walrus_list_append(target->command_list, p->prev);

        p = p->prev;
    }

    target->start = walrus_list_last(target->command_list);
}

void walrus_fg_compile(Walrus_FrameGraph *graph)
{
    // TODO: some prerender optimization?

    walrus_hash_table_foreach(graph->pipelines, construct_pipeline_command, NULL);
}

static void foreach_node_execute(void *ptr, void *userdata)
{
    Walrus_FrameNode  *node  = ptr;
    Walrus_FrameGraph *graph = userdata;
    node->func(graph, node);
}

void walrus_fg_execute(Walrus_FrameGraph *graph, char const *name)
{
    Walrus_FramePipeline *target = walrus_fg_lookup_pipeline(graph, name);

    Walrus_List *p = target->start;
    walrus_assert(p != NULL);

    while (p != NULL) {
        Walrus_FramePipeline *cur = p->data;
        walrus_array_foreach(cur->nodes, foreach_node_execute, graph);
        p = p->prev;
    }
}
