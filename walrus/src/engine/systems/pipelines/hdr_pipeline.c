#include <engine/systems/pipelines/hdr_pipeline.h>
#include <engine/deferred_renderer.h>
#include <core/log.h>
#include <core/macro.h>

static void hdr_pass(Walrus_FrameGraph *graph, Walrus_FrameNode const *node)
{
    walrus_unused(graph);
    walrus_unused(node);
    walrus_renderer_submit_hdr();

    walrus_renderer_submit_backbuffer();
}

Walrus_FramePipeline *walrus_hdr_pipeline_add(Walrus_FrameGraph *graph, char const *name)
{
    Walrus_FramePipeline *pipeline = walrus_fg_add_pipeline(graph, name);
    walrus_fg_add_node(pipeline, hdr_pass, "HDR");
    return pipeline;
}
