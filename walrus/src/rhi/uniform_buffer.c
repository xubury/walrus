#include "uniform_buffer.h"

#include <core/memory.h>
#include <core/assert.h>
#include <core/log.h>

#include <string.h>

UniformBuffer *uniform_buffer_create(u64 size)
{
    UniformBuffer *buffer = walrus_malloc(offsetof(UniformBuffer, data) + size);
    buffer->pos                  = 0;
    buffer->size                 = size;
    return buffer;
}

void uniform_buffer_update(UniformBuffer **buffer, u32 threshold, u32 grow)
{
    UniformBuffer *ptr = *buffer;

    u32 space_left = ptr->size - ptr->pos;
    if (threshold >= space_left) {
        walrus_trace("uniform buffer resize");
        u32 new_size = ptr->size + grow;
        ptr          = walrus_realloc(ptr, offsetof(UniformBuffer, data) + new_size);
        ptr->size    = new_size;
        *buffer      = ptr;
    }
}

void uniform_buffer_destroy(UniformBuffer *buffer)
{
    walrus_free(buffer);
}

void uniform_buffer_write(UniformBuffer *buffer, void const *data, u64 size)
{
    walrus_assert_msg(buffer->pos + size < buffer->size, "uniform_buffer_write go out of bound!");

    if (buffer->pos + size < buffer->size) {
        memcpy(buffer->data + buffer->pos, data, size);
        buffer->pos += size;
    }
}
void uniform_buffer_write_value(UniformBuffer *buffer, u64 value)
{
    uniform_buffer_write(buffer, &value, sizeof(u64));
}

void const *uniform_buffer_read(UniformBuffer *buffer, u64 size)
{
    walrus_assert_msg(buffer->pos < buffer->size, "uniform_buffer_read out of bound: %d (size: %d)", buffer->pos,
                      buffer->size);

    void const *data = buffer->data + buffer->pos;
    buffer->pos += size;

    return data;
}

u64 uniform_buffer_read_value(UniformBuffer *buffer)
{
    return *(u64 *)uniform_buffer_read(buffer, sizeof(u64));
}

void uniform_buffer_start(UniformBuffer *buffer, u32 pos)
{
    buffer->pos = pos;
}

void uniform_buffer_finish(UniformBuffer *buffer)
{
    uniform_buffer_write_value(buffer, UNIFORM_BUFFER_END);
    buffer->pos = 0;
}

void uniform_buffer_write_uniform(UniformBuffer *buffer, Walrus_UniformType type, Walrus_UniformHandle handle,
                                  u32 offset, u32 size, void const *data)
{
    uniform_buffer_write_value(buffer, uniform_encode_op(type, handle.id, 0));
    uniform_buffer_write_value(buffer, offset);
    uniform_buffer_write_value(buffer, size);
    uniform_buffer_write(buffer, data, size);
}

void uniform_buffer_write_uniform_handle(UniformBuffer *buffer, Walrus_UniformType type, u32 loc,
                                         Walrus_UniformHandle handle, u8 num)
{
    u64 op = uniform_encode_op(type, loc, num);
    uniform_buffer_write_value(buffer, op);
    uniform_buffer_write(buffer, &handle, sizeof(handle));
}

#define OP_TYPE_NBITS (8)
#define OP_TYPE_SHIFT (64 - OP_TYPE_NBITS)
#define OP_TYPE_MASK  ((((u64)1 << OP_TYPE_NBITS) - 1) << OP_TYPE_SHIFT)

#define OP_LOC_NBITS (32)
#define OP_LOC_SHIFT (OP_TYPE_SHIFT - OP_LOC_NBITS)
#define OP_LOC_MASK  ((((u64)1 << OP_LOC_NBITS) - 1) << OP_LOC_SHIFT)

#define OP_NUM_NBITS (8)
#define OP_NUM_SHIFT (OP_LOC_SHIFT - OP_NUM_NBITS)
#define OP_NUM_MASK  ((((u64)1 << OP_NUM_NBITS) - 1) << OP_NUM_SHIFT)

u64 uniform_encode_op(Walrus_UniformType type, u32 loc, u8 num)
{
    const u64 _type = (u64)type << OP_TYPE_SHIFT;
    const u64 _loc  = (u64)loc << OP_LOC_SHIFT;
    const u64 _num  = (u64)num << OP_NUM_SHIFT;
    return _type | _loc | _num;
}

void uniform_decode_op(Walrus_UniformType *type, u32 *loc, u8 *num, u64 op)
{
    if (type) {
        const u64 _type = (op & OP_TYPE_MASK) >> OP_TYPE_SHIFT;
        *type           = _type;
    }
    if (num) {
        const u64 _num = (op & OP_NUM_MASK) >> OP_NUM_SHIFT;
        *num           = _num;
    }
    if (loc) {
        const u64 _loc = (op & OP_LOC_MASK) >> OP_LOC_SHIFT;
        *loc           = _loc;
    }
}
