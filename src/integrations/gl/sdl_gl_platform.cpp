#include "platform/gpu.h"

#include <SDL.h>

#define VX_GL_IMPLEMENTATION
#include "gl.h"

// NOTE(vinht): Quote from ARB_base_instance extension page:
//
// 1) Does <baseinstance> offset gl_InstanceID?
//
// RESOLVED: No. gl_InstanceID always starts from zero and counts up by
// one for each instance rendered. If the shader author requires the
// actual value of the instance index, including the base instance, they
// must pass the base instance as a uniform. In OpenGL, the vertex
// attribute divisors are not passed implicitly to the shader anyway, so
// the shader writer will need to take care of this regardless.
//
// To emulate how the base_instance works in ANY OTHER API, we must pass
// it in ourselves.

#define VX_BASE_INSTANCE_BINDING_SLOT 32

namespace vx
{
namespace
{
struct gl_buffer
{
    u32 object;
    u32 target;
};

static_assert(sizeof(gl_buffer) == sizeof(uptr), "gl_buffer is not the size of a pointer");

struct gl_texture
{
    u32 object;
    u32 target;
};

static_assert(sizeof(gl_texture) == sizeof(uptr), "gl_texture is not the size of a pointer");

struct gl_sampler
{
    u32 object;
    u32 padding;
};

static_assert(sizeof(gl_sampler) == sizeof(uptr), "gl_sampler is not the size of a pointer");

struct gl_shader
{
    u32 object;
    u32 padding;
};

static_assert(sizeof(gl_shader) == sizeof(uptr), "gl_shader is not the size of a pointer");

struct gl_pipeline
{
    u32 program;

    bool depth_test_enabled;
    bool depth_write_enabled;
    bool culling_enabled;
    bool blend_enabled;

    // TODO: blend function, comparison function?
};

static_assert(sizeof(gl_pipeline) == sizeof(uptr), "gl_pipeline is not the size of a pointer");

struct gl_vertex_attribute
{
    i32 index;
    i32 elements;
    u32 type;
    u8 normalized;
    const void* offset;
};

struct gl_vertex_descriptor
{
    gl_vertex_attribute* attributes;
    u32 attribute_count;
    u32 stride;
};

struct gl_device
{
    SDL_GLContext context;

    int2 display_size;
    float2 display_scale;

    gl_pipeline current_pipeline;
};

i32 gpu_convert_enum(gpu_buffer_type type)
{
    switch (type)
    {
        case gpu_buffer_type::vertex:
            return GL_ARRAY_BUFFER;
        case gpu_buffer_type::index:
            return GL_ELEMENT_ARRAY_BUFFER;
        case gpu_buffer_type::constant:
            return GL_SHADER_STORAGE_BUFFER;
        default:
            fatal("Invalid gpu_buffer_type value: %i", int(type));
    }
}

i32 gpu_convert_enum(gpu_index_type type)
{
    switch (type)
    {
        case gpu_index_type::u16:
            return GL_UNSIGNED_SHORT;
        case gpu_index_type::u32:
            return GL_UNSIGNED_INT;
        default:
            fatal("Invalid gpu_index_type value: %i", int(type));
    }
}

gl_buffer gpu_convert_handle(gpu_buffer* buffer) { return (gl_buffer&)buffer; }

gl_texture gpu_convert_handle(gpu_texture* texture) { return (gl_texture&)texture; }

gl_sampler gpu_convert_handle(gpu_sampler* sampler) { return (gl_sampler&)sampler; }

gl_shader gpu_convert_handle(gpu_shader* shader) { return (gl_shader&)shader; }

gl_pipeline gpu_convert_handle(gpu_pipeline* pipeline) { return (gl_pipeline&)pipeline; }

i32 gpu_convert_enum(gpu_filter_mode mode)
{
    switch (mode)
    {
        case gpu_filter_mode::nearest:
            return GL_NEAREST;
        case gpu_filter_mode::linear:
            return GL_LINEAR;
        default:
            fatal("Invalid gpu_filter_mode value: %i", int(mode));
    }
}

i32 gpu_convert_enum(gpu_shader_type type)
{
    switch (type)
    {
        case gpu_shader_type::vertex:
            return GL_VERTEX_SHADER;
        case gpu_shader_type::fragment:
            return GL_FRAGMENT_SHADER;
        default:
            fatal("Invalid gpu_shader_type value: %i", int(type));
    }
}

i32 gpu_convert_enum(gpu_primitive_type type)
{
    switch (type)
    {
        case gpu_primitive_type::point:
            return GL_POINTS;
        case gpu_primitive_type::line:
            return GL_LINES;
        case gpu_primitive_type::line_strip:
            return GL_LINE_STRIP;
        case gpu_primitive_type::triangle:
            return GL_TRIANGLES;
        case gpu_primitive_type::triangle_strip:
            return GL_TRIANGLE_STRIP;
        default:
            fatal("Invalid gpu_primitive_type value: %i", int(type));
    }
}

void set_capability(u32 capability, bool enabled)
{
    if (enabled)
        glEnable(capability);
    else
        glDisable(capability);
}
}

void platform_init(platform* platform, const char* title, int2 initial_size)
{
    SDL_Window* sdl_window;
    SDL_GLContext sdl_gl_context;

    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

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

    if (!sdl_gl_context)
        fatal("OpenGL context creation failed!");

    vx_gl_init(SDL_GL_GetProcAddress);

    int2 display_size;
    int2 drawable_size;
    SDL_GetWindowSize(sdl_window, &display_size.x, &display_size.y);
    SDL_GL_GetDrawableSize(sdl_window, &drawable_size.x, &drawable_size.y);

    gl_device* device = (gl_device*)std::calloc(1, sizeof(gl_device));

    device->context = sdl_gl_context;
    device->display_size = display_size;
    device->display_scale = float2(
        display_size.x > 0 ? ((float)drawable_size.x / display_size.x) : 0.f,
        display_size.y > 0 ? ((float)drawable_size.y / display_size.y) : 0.f);

    platform->window = sdl_window;
    platform->gpu = (gpu_device*)device;

    //
    // global gl state
    //

    u32 dummy_vao;
    glGenVertexArrays(1, &dummy_vao);
    glBindVertexArray(dummy_vao);

    glEnable(GL_FRAMEBUFFER_SRGB);
    glFrontFace(GL_CCW);
}

void platform_quit(platform* platform)
{
    SDL_Window* sdl_window;
    SDL_GLContext sdl_gl_context;

    sdl_window = (SDL_Window*)platform->window;
    sdl_gl_context = ((gl_device*)platform->gpu)->context;

    SDL_GL_DeleteContext(sdl_gl_context);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
}

void platform_frame_begin(platform* platform) { (void)platform; }

void platform_frame_end(platform* platform)
{
    SDL_Window* sdl_window;
    sdl_window = (SDL_Window*)platform->window;
    SDL_GL_SwapWindow(sdl_window);
}

gpu_buffer* gpu_buffer_create(gpu_device* /*gpu*/, usize size, gpu_buffer_type type)
{
    gl_buffer buffer = {};
    glGenBuffers(1, &buffer.object);
    if (buffer.object == 0)
        fatal("Failed to create buffer!");

    buffer.target = gpu_convert_enum(type);

    glBindBuffer(buffer.target, buffer.object);
    glBufferData(buffer.target, size, nullptr, GL_STATIC_DRAW);

    return (gpu_buffer*)(*(uptr*)&buffer);
}

void gpu_buffer_update(
    gpu_device* /*gpu*/,
    gpu_buffer* buffer_handle,
    void* data,
    usize size,
    usize offset)
{
    gl_buffer buffer = gpu_convert_handle(buffer_handle);

    glBindBuffer(buffer.target, buffer.object);
    glBufferSubData(buffer.target, iptr(offset), size, data);
}

void gpu_buffer_destroy(gpu_device* /*gpu*/, gpu_buffer* buffer)
{
    if (!buffer)
        return;

    gl_buffer gl_buffer = gpu_convert_handle(buffer);
    glDeleteBuffers(1, &gl_buffer.object);
}

gpu_texture* gpu_texture_create(
    gpu_device* /*gpu*/,
    u32 width,
    u32 height,
    gpu_pixel_format format,
    void* data)
{
    gl_texture texture = {};
    glGenTextures(1, &texture.object);
    if (!texture.object)
        fatal("Failed to create texture!");

    // TODO: texture type
    texture.target = GL_TEXTURE_2D;
    glBindTexture(texture.target, texture.object);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    u32 px_internal_format = 0u;
    switch (format)
    {
        case gpu_pixel_format::rgba8_unorm:
            px_internal_format = GL_RGBA8;
            break;
        case gpu_pixel_format::rgba8_unorm_srgb:
            px_internal_format = GL_SRGB8_ALPHA8;
            break;
        default:
            fatal("Unknown gpu_pixel_format (%d)", (int)format);
    }

    u32 px_format = 0u;
    switch (format)
    {
        case gpu_pixel_format::rgba8_unorm:
        case gpu_pixel_format::rgba8_unorm_srgb:
            px_format = GL_RGBA;
            break;
        default:
            fatal("Unknown gpu_pixel_format (%d)", (int)format);
    }

    u32 px_type = 0u;
    switch (format)
    {
        case gpu_pixel_format::rgba8_unorm:
        case gpu_pixel_format::rgba8_unorm_srgb:
            px_type = GL_UNSIGNED_BYTE;
            break;
        default:
            fatal("Unknown gpu_pixel_format (%d)", (int)format);
    }

    glTexImage2D(texture.target, 0, px_internal_format, width, height, 0, px_format, px_type, data);

    return (gpu_texture*)(*(uptr*)&texture);
}

void gpu_texture_destroy(gpu_device* /*gpu*/, gpu_texture* texture_handle)
{
    gl_texture texture = gpu_convert_handle(texture_handle);
    glDeleteTextures(1, &texture.object);
}

gpu_sampler* gpu_sampler_create(
    gpu_device* /*gpu*/,
    gpu_filter_mode min,
    gpu_filter_mode mag,
    gpu_filter_mode /*mip*/)
{
    gl_sampler sampler = {};
    glGenSamplers(1, &sampler.object);
    if (!sampler.object)
        fatal("Sampler creation failed!");

    // TODO(vinht): Mipmap filtering modes.
    // TODO(vinht): Addressing modes.
    glSamplerParameteri(sampler.object, GL_TEXTURE_MAG_FILTER, gpu_convert_enum(mag));
    glSamplerParameteri(sampler.object, GL_TEXTURE_MIN_FILTER, gpu_convert_enum(min));
    glSamplerParameteri(sampler.object, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler.object, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return (gpu_sampler*)(*(uptr*)&sampler);
}

void gpu_sampler_destroy(gpu_device* /*gpu*/, gpu_sampler* sampler_handle)
{
    gl_sampler sampler = gpu_convert_handle(sampler_handle);
    glDeleteSamplers(1, &sampler.object);
}

gpu_vertex_desc* gpu_vertex_desc_create(
    gpu_device* /*gpu*/,
    gpu_vertex_desc_attribute* attributes,
    u32 attribute_count,
    u32 vertex_stride)
{
    gl_vertex_attribute* gl_attributes =
        (gl_vertex_attribute*)std::calloc(attribute_count, sizeof(gl_vertex_attribute));
    uptr offset = 0u;
    for (u32 i = 0u; i < attribute_count; i++)
    {
        u32 element_count = 0u;
        switch (attributes[i].format)
        {
            case gpu_vertex_format::float1:
                element_count = 1u;
                break;
            case gpu_vertex_format::float2:
                element_count = 2u;
                break;
            case gpu_vertex_format::float3:
                element_count = 3u;
                break;
            case gpu_vertex_format::float4:
                element_count = 4u;
                break;
            case gpu_vertex_format::rgba8_unorm:
                element_count = 4u;
                break;
            default:
                break;
        }

        u32 element_type = 0u;
        switch (attributes[i].format)
        {
            case gpu_vertex_format::float1:
            case gpu_vertex_format::float2:
            case gpu_vertex_format::float3:
            case gpu_vertex_format::float4:
                element_type = GL_FLOAT;
                break;
            case gpu_vertex_format::rgba8_unorm:
                element_type = GL_UNSIGNED_BYTE;
                break;
            default:
                break;
        }

        uptr element_size = 0u;
        switch (attributes[i].format)
        {
            case gpu_vertex_format::float1:
                element_size = 4u;
                break;
            case gpu_vertex_format::float2:
                element_size = 8u;
                break;
            case gpu_vertex_format::float3:
                element_size = 12u;
                break;
            case gpu_vertex_format::float4:
                element_size = 16u;
                break;
            case gpu_vertex_format::rgba8_unorm:
                element_size = 4u;
                break;
            default:
                break;
        }

        gl_attributes[i].index = attributes[i].buffer_index;
        gl_attributes[i].elements = element_count;
        gl_attributes[i].normalized =
            attributes[i].format == gpu_vertex_format::rgba8_unorm ? GL_TRUE : GL_FALSE;
        gl_attributes[i].type = element_type;
        gl_attributes[i].offset = (const void*)offset;

        offset += element_size;
    }

    gl_vertex_descriptor* descriptor =
        (gl_vertex_descriptor*)std::calloc(1, sizeof(gl_vertex_descriptor));
    descriptor->attributes = gl_attributes;
    descriptor->attribute_count = attribute_count;
    descriptor->stride = vertex_stride;

    return (gpu_vertex_desc*)descriptor;
}

void gpu_vertex_desc_destroy(gpu_device* /*gpu*/, gpu_vertex_desc* vertex_desc)
{
    gl_vertex_descriptor* descriptor = (gl_vertex_descriptor*)vertex_desc;
    std::free(descriptor->attributes);
    std::free(descriptor);
}

gpu_shader* gpu_shader_create(
    gpu_device* /*gpu*/,
    gpu_shader_type type,
    void* data,
    usize /*size*/,
    const char* /*main_function*/)
{
    const char* prelude =
        "#version 440 core\n"
        "#define VX_VERTEX_SHADER 0\n"
        "#define VX_FRAGMENT_SHADER 1\n"
        "layout(location = " vx_xstr(VX_BASE_INSTANCE_BINDING_SLOT) ") uniform int VX_BASE_INSTANCE;\n"
        "#define VX_INSTANCE_ID (gl_InstanceID + VX_BASE_INSTANCE)\n";
    const char* shader_types[] = {
        "#define VX_SHADER VX_VERTEX_SHADER\n", "#define VX_SHADER VX_FRAGMENT_SHADER\n",
    };
    const char* prologue = "#line 1\n";
    const char* sources[] = {prelude, shader_types[int(type)], prologue, (char*)data};

    gl_shader shader = {};
    shader.object = glCreateShader(gpu_convert_enum(type));
    if (!shader.object)
        fatal("Failed to create shader!");

    glShaderSource(shader.object, vx_countof(sources), (char**)sources, 0);
    glCompileShader(shader.object);

    i32 status;
    glGetShaderiv(shader.object, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        // TODO: log error here
        char buf[2048];
        glGetShaderInfoLog(shader.object, sizeof(buf) - 1u, 0, buf);
        fatal("Shader compilation failed: %s", buf);
    }

    return (gpu_shader*)(*(uptr*)&shader);
}

void gpu_shader_destroy(gpu_device* /*gpu*/, gpu_shader* shader)
{
    if (!shader)
        return;

    gl_shader gl_shader = gpu_convert_handle(shader);
    glDeleteShader(gl_shader.object);
}

gpu_pipeline* gpu_pipeline_create(
    gpu_device* /*gpu*/,
    gpu_shader* vertex_shader_handle,
    gpu_shader* fragment_shader_handle,
    const gpu_pipeline_options& options)
{
    gl_shader vertex_shader = gpu_convert_handle(vertex_shader_handle);
    gl_shader fragment_shader = gpu_convert_handle(fragment_shader_handle);

    gl_pipeline pipeline = {};
    pipeline.blend_enabled = options.blend_enabled;
    pipeline.culling_enabled = options.culling_enabled;
    pipeline.depth_test_enabled = options.depth_test_enabled;
    pipeline.depth_write_enabled = options.depth_write_enabled;
    pipeline.program = glCreateProgram();

    if (!pipeline.program)
        fatal("Failed to create shader program!");

    glAttachShader(pipeline.program, vertex_shader.object);
    glAttachShader(pipeline.program, fragment_shader.object);

    glLinkProgram(pipeline.program);

    i32 status;
    glGetProgramiv(pipeline.program, GL_LINK_STATUS, &status);

    if (status == GL_FALSE)
    {
        char buf[2048];
        glGetProgramInfoLog(pipeline.program, sizeof(buf) - 1u, 0, buf);
        fatal("Program linking failed: %s", buf);
    }

    return (gpu_pipeline*)(*(uptr*)(&pipeline));
}

void gpu_pipeline_destroy(gpu_device* /*gpu*/, gpu_pipeline* pipeline_handle)
{
    if (!pipeline_handle)
        return;

    gl_pipeline pipeline = gpu_convert_handle(pipeline_handle);
    glDeleteProgram(pipeline.program);
}

gpu_channel* gpu_channel_open(gpu_device* gpu)
{
    // TODO(vinht): We have to make sure the state is "clean" at this point
    // because we don't have command buffers to precisely control this.
    set_capability(GL_BLEND, false);
    set_capability(GL_CULL_FACE, false);
    set_capability(GL_DEPTH_TEST, false);
    set_capability(GL_SCISSOR_TEST, false);
    glDepthMask(true);
    return (gpu_channel*)gpu;
}

void gpu_channel_close(gpu_device* gpu, gpu_channel* channel)
{
    (void)gpu;
    (void)channel;
}

void gpu_channel_clear_cmd(gpu_channel* channel, gpu_clear_cmd_args* args)
{
    // TODO(vinht): Append to command buffer.
    (void)channel;

    float4 c = args->color;
    glClearColor(c.x, c.y, c.z, c.w);
    glClearDepth(args->depth);
    glClearStencil(args->stencil);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void gpu_channel_set_buffer_cmd(gpu_channel* /*channel*/, gpu_buffer* buffer_handle, u32 index)
{
    gl_buffer buffer = gpu_convert_handle(buffer_handle);
    if (buffer.target == GL_SHADER_STORAGE_BUFFER)
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, buffer.object);
    else
        glBindBuffer(buffer.target, buffer.object);
}

void gpu_channel_set_vertex_desc_cmd(gpu_channel* /*channel*/, gpu_vertex_desc* vertex_desc)
{
    gl_vertex_descriptor* descriptor = (gl_vertex_descriptor*)vertex_desc;

    for (u32 i = 0u; i < descriptor->attribute_count; i++)
    {
        const gl_vertex_attribute& attrib = descriptor->attributes[i];
        glEnableVertexAttribArray(attrib.index);
        glVertexAttribPointer(
            attrib.index,
            attrib.elements,
            attrib.type,
            attrib.normalized,
            descriptor->stride,
            (void*)attrib.offset);
    }
}

void gpu_channel_set_texture_cmd(gpu_channel* /*channel*/, gpu_texture* texture_handle, u32 index)
{
    gl_texture texture = gpu_convert_handle(texture_handle);
    glActiveTexture(GL_TEXTURE0 + index);
    glBindTexture(texture.target, texture.object);
}

void gpu_channel_set_sampler_cmd(gpu_channel* /*channel*/, gpu_sampler* sampler_handle, u32 index)
{
    gl_sampler sampler = gpu_convert_handle(sampler_handle);
    glBindSampler(index, sampler.object);
}

void gpu_channel_set_pipeline_cmd(gpu_channel* channel, gpu_pipeline* pipeline_handle)
{
    gl_device* device = (gl_device*)channel;
    gl_pipeline pipeline = gpu_convert_handle(pipeline_handle);
    device->current_pipeline = pipeline;

    set_capability(GL_BLEND, pipeline.blend_enabled);
    set_capability(GL_CULL_FACE, pipeline.culling_enabled);
    set_capability(GL_DEPTH_TEST, pipeline.depth_test_enabled);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(pipeline.depth_write_enabled);

    // TODO: these shouldn't be hardcoded here
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(pipeline.program);
}

void gpu_channel_set_scissor_cmd(gpu_channel* channel, gpu_scissor_rect* rect)
{
    gl_device* device = (gl_device*)channel;
    // The OpenGL screen coordinates are flipped w.r.t. to the y-axis
    // y-zero is at the top of the screen
    set_capability(GL_SCISSOR_TEST, true);
    int fb_height = int(device->display_size.y * device->display_scale.y);
    glScissor(i32(rect->x), fb_height - int(rect->y + rect->h), i32(rect->w), i32(rect->h));
}

void gpu_channel_set_viewport_cmd(gpu_channel* /*channel*/, gpu_viewport* viewport)
{
    glViewport(i32(viewport->x), i32(viewport->y), i32(viewport->w), i32(viewport->h));
}

void gpu_channel_draw_primitives_cmd(
    gpu_channel* /*channel*/,
    gpu_primitive_type primitive_type,
    u32 vertex_start,
    u32 vertex_count,
    u32 instance_count,
    u32 base_instance)
{
    glUniform1i(VX_BASE_INSTANCE_BINDING_SLOT, base_instance);
    glDrawArraysInstancedBaseInstance(
        gpu_convert_enum(primitive_type),
        vertex_start,
        vertex_count,
        instance_count,
        base_instance);
}

void gpu_channel_draw_indexed_primitives_cmd(
    gpu_channel* /*channel*/,
    gpu_primitive_type primitive_type,
    u32 index_count,
    gpu_index_type index_type,
    gpu_buffer* index_buffer_handle,
    u32 index_byte_offset,
    u32 instance_count,
    i32 base_vertex,
    u32 base_instance)
{
    gl_buffer index_buffer = gpu_convert_handle(index_buffer_handle);
    if (index_buffer.target != GL_ELEMENT_ARRAY_BUFFER)
        fatal("The index buffer provided was not created as an index buffer!");
    glBindBuffer(index_buffer.target, index_buffer.object);
    glUniform1i(VX_BASE_INSTANCE_BINDING_SLOT, base_instance);
    glDrawElementsInstancedBaseVertexBaseInstance(
        gpu_convert_enum(primitive_type),
        index_count,
        gpu_convert_enum(index_type),
        (void*)uptr(index_byte_offset),
        instance_count,
        base_vertex,
        base_instance);
}
}
