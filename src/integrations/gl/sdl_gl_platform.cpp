#include "platform/gpu.h"
#include "common/error.h"

#include "SDL.h"
#include "GL/gl3w.h"

namespace
{
struct gl_device
{
    SDL_GLContext ctx;
};
}

namespace vx
{
void platform_init(platform* platform, const char* title, int2 initial_size)
{
    SDL_Window* sdl_window;
    SDL_GLContext sdl_gl_context;

    SDL_Init(SDL_INIT_VIDEO);

    sdl_window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        initial_size.x,
        initial_size.y,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!sdl_window)
        fatal("SDL_CreateWindow failed with error: %s", SDL_GetError());

    sdl_gl_context = SDL_GL_CreateContext(sdl_window);

    if (gl3wInit() == -1)
        fatal("gl3wInit failed");

    platform->window = sdl_window;
    platform->gpu = (gpu_device*)sdl_gl_context;
}

void platform_quit(platform* platform)
{
    SDL_Window* sdl_window;
    SDL_GLContext sdl_gl_context;

    sdl_window = (SDL_Window*)platform->window;
    sdl_gl_context = (SDL_GLContext)platform->gpu;

    SDL_GL_DeleteContext(sdl_gl_context);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
}

void platform_frame_begin(platform* /*platform*/) {}

void platform_frame_end(platform* platform)
{
    SDL_Window* sdl_window;
    sdl_window = (SDL_Window*)platform->window;
    SDL_GL_SwapWindow(sdl_window);
}

gpu_buffer* gpu_buffer_create(gpu_device* gpu, usize size)
{
    (void)gpu;
    (void)size;
    return nullptr;
}

void gpu_buffer_update(gpu_device* gpu, gpu_buffer* buffer, void* data, usize size, usize offset)
{
    (void)gpu;
    (void)buffer;
    (void)data;
    (void)size;
    (void)offset;
}

void gpu_buffer_destroy(gpu_device* gpu, gpu_buffer* buffer)
{
    (void)gpu;
    (void)buffer;
}

gpu_texture* gpu_texture_create(
    gpu_device* gpu,
    u32 width,
    u32 height,
    gpu_pixel_format format,
    void* data)
{
    (void)gpu;
    (void)width;
    (void)height;
    (void)format;
    (void)data;
    return nullptr;
}

void gpu_texture_destroy(gpu_device* gpu, gpu_texture* texture)
{
    (void)gpu;
    (void)texture;
}

gpu_sampler* gpu_sampler_create(
    gpu_device* gpu,
    gpu_filter_mode min,
    gpu_filter_mode mag,
    gpu_filter_mode mip)
{
    (void)gpu;
    (void)min;
    (void)mag;
    (void)mip;
    return nullptr;
}

void gpu_sampler_destroy(gpu_device* gpu, gpu_sampler* sampler)
{
    (void)gpu;
    (void)sampler;
}

gpu_vertex_desc* gpu_vertex_desc_create(
    gpu_device* gpu,
    gpu_vertex_desc_attribute* attributes,
    u32 attribute_count,
    u32 vertex_stride)
{
    (void)gpu;
    (void)attributes;
    (void)attribute_count;
    (void)vertex_stride;
    return nullptr;
}

void gpu_vertex_desc_destroy(gpu_device* gpu, gpu_vertex_desc* vertex_desc)
{
    (void)gpu;
    (void)vertex_desc;
}

gpu_shader* gpu_shader_create(
    gpu_device* gpu,
    gpu_shader_type type,
    void* data,
    usize size,
    const char* main_function)
{
    (void)gpu;
    (void)type;
    (void)data;
    (void)size;
    (void)main_function;
    return nullptr;
}

void gpu_shader_destroy(gpu_device* gpu, gpu_shader* shader)
{
    (void)gpu;
    (void)shader;
}

gpu_pipeline* gpu_pipeline_create(
    gpu_device* gpu,
    gpu_shader* vertex_shader,
    gpu_shader* fragment_shader)
{
    (void)gpu;
    (void)vertex_shader;
    (void)fragment_shader;
    return nullptr;
}

void gpu_pipeline_destroy(gpu_device* gpu, gpu_pipeline* pipeline)
{
    (void)gpu;
    (void)pipeline;
}

gpu_channel* gpu_channel_open(gpu_device* gpu)
{
    (void)gpu;
    return nullptr;
}

void gpu_channel_close(gpu_device* gpu, gpu_channel* channel)
{
    (void)gpu;
    (void)channel;
}

void gpu_channel_clear_cmd(gpu_channel* channel, gpu_clear_cmd_args* args)
{
    (void)channel;
    (void)args;
}

void gpu_channel_set_buffer_cmd(gpu_channel* channel, gpu_buffer* buffer, u32 index)
{
    (void)channel;
    (void)buffer;
    (void)index;
}

void gpu_channel_set_texture_cmd(gpu_channel* channel, gpu_texture* texture, u32 index)
{
    (void)channel;
    (void)texture;
    (void)index;
}

void gpu_channel_set_sampler_cmd(gpu_channel* channel, gpu_sampler* sampler, u32 index)
{
    (void)channel;
    (void)sampler;
    (void)index;
}

void gpu_channel_set_pipeline_cmd(gpu_channel* channel, gpu_pipeline* pipeline)
{
    (void)channel;
    (void)pipeline;
}

void gpu_channel_set_scissor_cmd(gpu_channel* channel, gpu_scissor_rect* rect)
{
    (void)channel;
    (void)rect;
}

void gpu_channel_set_viewport_cmd(gpu_channel* channel, gpu_viewport* viewport)
{
    (void)channel;
    (void)viewport;
}

void gpu_channel_draw_primitives_cmd(
    gpu_channel* channel,
    gpu_primitive_type primitive_type,
    u32 vertex_start,
    u32 vertex_count)
{
    (void)channel;
    (void)primitive_type;
    (void)vertex_start;
    (void)vertex_count;
}

void gpu_channel_draw_indexed_primitives_cmd(
    gpu_channel* channel,
    gpu_primitive_type primitive_type,
    u32 index_count,
    gpu_index_type index_type,
    gpu_buffer* index_buffer,
    u32 index_byte_offset)
{
    (void)channel;
    (void)primitive_type;
    (void)index_count;
    (void)index_type;
    (void)index_buffer;
    (void)index_byte_offset;
}
}
