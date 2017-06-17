#pragma once

#include "common/aliases.h"
#include "platform/native_platform.h"

namespace vx
{
struct gpu_buffer;
struct gpu_texture;
struct gpu_sampler;
struct gpu_vertex_desc;
struct gpu_shader;
struct gpu_pipeline;
struct gpu_channel;

enum gpu_vertex_format
{
    gpu_vertex_format_float,
    gpu_vertex_format_float2,
    gpu_vertex_format_float3,
    gpu_vertex_format_float4,

    gpu_vertex_format_rgba8_unorm,
};

enum gpu_index_type
{
    gpu_index_type_u16,
    gpu_index_type_u32,
};

enum gpu_pixel_format
{
    gpu_pixel_format_rgba8_unorm,
};

enum gpu_filter_mode
{
    gpu_filter_mode_nearest,
    gpu_filter_mode_linear,
};

enum gpu_shader_type
{
    gpu_shader_type_vertex,
    gpu_shader_type_fragment,
};

enum gpu_primitive_type
{
    gpu_primitive_type_point,
    gpu_primitive_type_line,
    gpu_primitive_type_line_strip,
    gpu_primitive_type_triangle,
    gpu_primitive_type_triangle_strip,
};

struct gpu_scissor_rect
{
    u32 x, y, w, h;
};

struct gpu_viewport
{
    float x, y, w, h, znear, zfar;
};

gpu_buffer* gpu_buffer_create(gpu_device* gpu, usize size);
void gpu_buffer_update(gpu_device* gpu, gpu_buffer* buffer, void* data, usize size, usize offset);
void gpu_buffer_destroy(gpu_device* gpu, gpu_buffer* buffer);

gpu_texture* gpu_texture_create(
    gpu_device* gpu,
    u32 width,
    u32 height,
    gpu_pixel_format format,
    void* data);
void gpu_texture_destroy(gpu_device* gpu, gpu_texture* texture);

gpu_sampler* gpu_sampler_create(
    gpu_device* gpu,
    gpu_filter_mode min,
    gpu_filter_mode mag,
    gpu_filter_mode mip);
void gpu_sampler_destroy(gpu_device* gpu, gpu_sampler* sampler);

struct gpu_vertex_desc_attribute
{
    gpu_vertex_format format;
    u32 buffer_index;
    usize offset;
};

gpu_vertex_desc* gpu_vertex_desc_create(
    gpu_device* gpu,
    gpu_vertex_desc_attribute* attributes,
    u32 attribute_count,
    u32 vertex_stride);
void gpu_vertex_desc_destroy(gpu_device* gpu, gpu_vertex_desc* vertex_desc);

gpu_shader* gpu_shader_create(
    gpu_device* gpu,
    gpu_shader_type type,
    void* data,
    usize size,
    const char* main_function);
void gpu_shader_destroy(gpu_device* gpu, gpu_shader* shader);

gpu_pipeline* gpu_pipeline_create(
    gpu_device* gpu,
    gpu_shader* vertex_shader,
    gpu_shader* fragment_shader);
void gpu_pipeline_destroy(gpu_device* gpu, gpu_pipeline* pipeline);

gpu_channel* gpu_channel_open(gpu_device* gpu);
void gpu_channel_close(gpu_device* gpu, gpu_channel* channel);

struct gpu_clear_cmd_args
{
    float4 color;
    float depth;
    u8 stencil;
};

void gpu_channel_clear_cmd(gpu_channel* channel, gpu_clear_cmd_args* args);
void gpu_channel_set_buffer_cmd(gpu_channel* channel, gpu_buffer* buffer, u32 index);
void gpu_channel_set_texture_cmd(gpu_channel* channel, gpu_texture* texture, u32 index);
void gpu_channel_set_sampler_cmd(gpu_channel* channel, gpu_sampler* sampler, u32 index);
void gpu_channel_set_pipeline_cmd(gpu_channel* channel, gpu_pipeline* pipeline);
void gpu_channel_set_scissor_cmd(gpu_channel* channel, gpu_scissor_rect* rect);
void gpu_channel_set_viewport_cmd(gpu_channel* channel, gpu_viewport* viewport);
void gpu_channel_draw_primitives_cmd(
    gpu_channel* channel,
    gpu_primitive_type primitive_type,
    u32 vertex_start,
    u32 vertex_count);
void gpu_channel_draw_indexed_primitives_cmd(
    gpu_channel* channel,
    gpu_primitive_type primitive_type,
    u32 index_count,
    gpu_index_type index_type,
    gpu_buffer* index_buffer,
    u32 index_byte_offset);
}
