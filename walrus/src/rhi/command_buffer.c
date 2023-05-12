#include "command_buffer.h"
#include <core/math.h>
#include <core/memory.h>
#include <core/assert.h>

#include <string.h>

void command_buffer_resize(CommandBuffer *buffer, u32 capacity)
{
    buffer->capacity = walrus_align_up(walrus_max(capacity, buffer->min_capacity), 1024);
    buffer->data     = (u8 *)walrus_realloc(buffer->data, buffer->capacity);
}

void command_buffer_init(CommandBuffer *buffer, u32 min_capacity)
{
    buffer->pos          = 0;
    buffer->size         = 0;
    buffer->min_capacity = walrus_align_up(min_capacity, 1024);
    command_buffer_resize(buffer, 0);
}

void command_buffer_shutdown(CommandBuffer *buffer)
{
    walrus_free(buffer->data);
}

void command_buffer_start(CommandBuffer *buffer)
{
    buffer->pos  = 0;
    buffer->size = 0;
}

void command_buffer_finish(CommandBuffer *buffer)
{
    Command cmd = COMMAND_END;
    command_buffer_write(buffer, Command, &cmd);
    buffer->size = buffer->pos;
    buffer->pos  = 0;
    if (buffer->size < buffer->min_capacity && buffer->capacity != buffer->min_capacity) {
        command_buffer_resize(buffer, 0);
    }
}

void command_buffer_reset(CommandBuffer *buffer)
{
    buffer->pos = 0;
}

void command_buffer_write_data(CommandBuffer *buffer, void const *data, u32 size)
{
    walrus_assert_msg(buffer->size == 0, "command_buffer_write called outside start/finish (size: %d)?", buffer->size);
    if (buffer->pos + size > buffer->capacity) {
        command_buffer_resize(buffer, walrus_max(buffer->pos + size, buffer->capacity + (16 << 10)));
    }
    memcpy(&buffer->data[buffer->pos], data, size);
    buffer->pos += size;
}

void command_buffer_read_data(CommandBuffer *buffer, void *data, u32 size)
{
    walrus_assert_msg(buffer->pos + size <= buffer->size, "command_buffer_read error(pos:%d-%d, size:%d)", buffer->pos,
                      buffer->pos + size, buffer->size);
    memcpy(data, &buffer->data[buffer->pos], size);
    buffer->pos += size;
}

void const *command_buffer_skip_data(CommandBuffer *buffer, u32 size)
{
    walrus_assert_msg(buffer->pos + size <= buffer->size, "command_buffer_skip error(pos:%d-%d, size:%d)", buffer->pos,
                      buffer->pos + size, buffer->size);

    void const *result = &buffer->data[buffer->pos];
    buffer->pos += size;
    return result;
}

void command_buffer_align(CommandBuffer *buffer, u32 align)
{
    u32 const mask = align - 1;
    u32 const pos  = (buffer->pos + mask) & (~mask);
    buffer->pos    = pos;
}
