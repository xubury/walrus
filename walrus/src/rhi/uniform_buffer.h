#pragma once

#include <rhi/type.h>

#define UNIFORM_BUFFER_END UINT64_MAX

typedef struct {
    u32 size;
    u32 pos;
    u8  data[1];
} UniformBuffer;

UniformBuffer* uniform_buffer_create(u64 size);

void uniform_buffer_update(UniformBuffer** buffer, u32 threshold, u32 grow);

void uniform_buffer_destroy(UniformBuffer* buffer);

void uniform_buffer_write(UniformBuffer* buffer, void const* data, u64 size);
void uniform_buffer_write_value(UniformBuffer* buffer, u64 value);

void const* uniform_buffer_read(UniformBuffer* buffer, u64 size);
u64         uniform_buffer_read_value(UniformBuffer* buffer);

void uniform_buffer_start(UniformBuffer* buffer, u32 pos);
void uniform_buffer_finish(UniformBuffer* buffer);

void uniform_buffer_write_uniform(UniformBuffer* buffer, Walrus_UniformType type, Walrus_UniformHandle handle,
                                  u32 offset, u32 size, void const* data);
void uniform_buffer_write_uniform_handle(UniformBuffer* buffer, Walrus_UniformType type, u32 loc,
                                         Walrus_UniformHandle handle, u8 num);

u64  uniform_encode_op(Walrus_UniformType type, u32 loc, u8 num);
void uniform_decode_op(Walrus_UniformType* type, u32* loc, u8* num, u64 op);
