#pragma once

#include <core/type.h>
#include <stdalign.h>

typedef struct {
    u8 *data;
    u32 pos;
    u32 size;
    u32 capacity;
    u32 min_capacity;
} CommandBuffer;

typedef enum {
    COMMAND_RENDERER_INIT,
    COMMAND_RENDERER_SHUTDOWN_BEGIN,
    COMMAND_CREATE_VERTEX_LAYOUT,
    COMMAND_CREATE_BUFFER,
    COMMAND_UPDATE_BUFFER,
    COMMAND_CREATE_SHADER,
    COMMAND_CREATE_PROGRAM,
    COMMAND_CREATE_TEXTURE,
    COMMAND_UPDATE_TEXTURE,
    COMMAND_RESIZE_TEXTURE,
    COMMAND_CREATE_FRAMEBUFFER,
    COMMAND_CREATE_UNIFORM,
    COMMAND_RESIZE_UNIFORM,
    COMMAND_UPDATE_VIEW_NAME,
    COMMAND_INVALIDATE_OCCLUSION_QUERY,
    COMMAND_SET_NAME,
    COMMAND_END,
    COMMAND_RENDERER_SHUTDOWN_END,
    COMMAND_DESTROY_VERTEX_LAYOUT,
    COMMAND_DESTROY_BUFFER,
    COMMAND_DESTROY_DYNAMICBUFFER,
    COMMAND_DESTROY_SHADER,
    COMMAND_DESTROY_PROGRAM,
    COMMAND_DESTROY_TEXTURE,
    COMMAND_DESTROY_FRAMEBUFFER,
    COMMAND_DESTROY_UNIFORM,
    COMMAND_READ_TEXTURE,
    COMMAND_READ_BUFFER,
} Command;

void command_buffer_init(CommandBuffer *buffer, u32 min_capacity);

void command_buffer_shutdown(CommandBuffer *buffer);

void command_buffer_start(CommandBuffer *buffer);

void command_buffer_finish(CommandBuffer *buffer);

void command_buffer_reset(CommandBuffer *buffer);

void command_buffer_write_data(CommandBuffer *buffer, void const *data, u32 size);

void command_buffer_read_data(CommandBuffer *buffer, void *data, u32 size);

void const *command_buffer_skip_data(CommandBuffer *buffer, u32 size);

void command_buffer_align(CommandBuffer *buffer, u32 align);

#define command_buffer_write(buffer, type, pval) \
    command_buffer_align(buffer, alignof(type)); \
    command_buffer_write_data(buffer, pval, sizeof(type));

#define command_buffer_read(buffer, type, pval)  \
    command_buffer_align(buffer, alignof(type)); \
    command_buffer_read_data(buffer, pval, sizeof(type));

#define command_buffer_skip(buffer, type)        \
    command_buffer_align(buffer, alignof(type)); \
    command_buffer_skip_data(buffer, sizeof(type));
