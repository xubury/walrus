#include <engine/rdg.h>
#include <core/memory.h>
#include <core/macro.h>
#include <core/string.h>
#include <core/log.h>
#include <core/assert.h>

void node_free(void *ptr)
{
    Walrus_RenderNode *node = ptr;
    walrus_str_free(node->name);
}

void walrus_fg_init(Walrus_FrameGraph *graph)
{
    graph->resources = walrus_hash_table_create(walrus_str_hash, walrus_str_equal);
    graph->nodes     = walrus_array_create_full(sizeof(Walrus_RenderNode), 0, node_free);
}

void walrus_fg_shutdown(Walrus_FrameGraph *graph)
{
    walrus_hash_table_destroy(graph->resources);
    walrus_array_destroy(graph->nodes);
}

void walrus_fg_add_node(Walrus_FrameGraph *graph, Walrus_RenderNodeCallback func, char const *name)
{
    Walrus_RenderNode node = {.name = walrus_str_dup(name), .index = walrus_array_len(graph->nodes), .func = func};

    walrus_array_append(graph->nodes, &node);
}

void walrus_fg_clear(Walrus_FrameGraph *graph)
{
    walrus_array_clear(graph->nodes);
}

void walrus_fg_write(Walrus_FrameGraph *graph, char const *name, u64 handle)
{
    walrus_hash_table_insert(graph->resources, walrus_str_dup(name), walrus_val_to_ptr(handle));
}

u64 walrus_fg_read(Walrus_FrameGraph *graph, char const *name)
{
    return walrus_ptr_to_val(walrus_hash_table_lookup(graph->resources, name));
}

void walrus_fg_compile(Walrus_FrameGraph *graph)
{
    walrus_unused(graph);
}

static void foreach_node_execute(void *ptr, void *userdata)
{
    Walrus_RenderNode *node = ptr;
    node->func(node, userdata);
}

void walrus_fg_execute(Walrus_FrameGraph *graph, void *userdata)
{
    walrus_array_foreach(graph->nodes, foreach_node_execute, userdata);
}
