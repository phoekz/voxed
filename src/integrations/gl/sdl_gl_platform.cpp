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
    gl_shader vertex_shader;
    gl_shader fragment_shader;
};

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

    gl_pipeline* current_pipeline;
};

i32 gpu_convert_enum(gpu_buffer_type type)
{
    switch (type)
    {
        case gpu_buffer_type::vertex:
            return GL_SHADER_STORAGE_BUFFER;
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

void debug_message_callback(
    i32 source,
    i32 type,
    u32 /*id*/,
    i32 severity,
    iptr /*length*/,
    const char* message,
    void* /*user*/)
{
    const char* source_str;
    const char* type_str;
    const char* severity_str;

    switch (source)
    {
        case GL_DEBUG_SOURCE_API:
            source_str = "api";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            source_str = "window_system";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            source_str = "shader_compiler";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            source_str = "third_party";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            source_str = "application";
            break;
        case GL_DEBUG_SOURCE_OTHER:
            source_str = "other";
            break;
        default:
            fatal("Unknown GL_DEBUG_SOURCE: %d", source);
    }

    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:
            type_str = "error";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            type_str = "deprecated_behavior";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            type_str = "undefined_behavior";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            type_str = "portability";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            type_str = "performance";
            break;
        case GL_DEBUG_TYPE_MARKER:
            type_str = "marker";
            break;
        case GL_DEBUG_TYPE_OTHER:
            type_str = "other";
            break;
        default:
            fatal("Unknown GL_DEBUG_TYPE: %d", type);
    }

    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:
            severity_str = "high";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            severity_str = "medium";
            break;
        case GL_DEBUG_SEVERITY_LOW:
            severity_str = "low";
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            severity_str = "notification";
            break;
        default:
            fatal("Unknown GL_DEBUG_SEVERITY: %d", severity);
    }

    fprintf(
        stderr,
        "OpenGL: src:%s type:%s severity:%s msg:%s\n",
        source_str,
        type_str,
        severity_str,
        message);
}

} // namespace

void platform_init(platform* platform, const char* title, int2 initial_size)
{
    SDL_Window* sdl_window;
    SDL_GLContext sdl_gl_context;

    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
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
    glCreateVertexArrays(1, &dummy_vao);
    glBindVertexArray(dummy_vao);

    glEnable(GL_FRAMEBUFFER_SRGB);
    glFrontFace(GL_CCW);

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(debug_message_callback, nullptr);
    // TODO(vinht): Some notifications could be useful to see, but the
    // defaults are too verbose to be useful. Perhaps do some filtering on
    // this?
    glDebugMessageControl(
        GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, false);
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
    glCreateBuffers(1, &buffer.object);
    if (buffer.object == 0)
        fatal("Failed to create buffer!");

    buffer.target = gpu_convert_enum(type);

    glNamedBufferStorage(buffer.object, size, nullptr, GL_DYNAMIC_STORAGE_BIT);

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

    glNamedBufferSubData(buffer.object, iptr(offset), size, data);
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
    glCreateTextures(GL_TEXTURE_2D, 1, &texture.object);
    if (!texture.object)
        fatal("Failed to create texture!");

    // TODO: texture type
    texture.target = GL_TEXTURE_2D;

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

    glTextureStorage2D(texture.object, 1, px_internal_format, width, height);
    glTextureSubImage2D(texture.object, 0, 0, 0, width, height, px_format, px_type, data);

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
    glCreateSamplers(1, &sampler.object);
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

gpu_shader* gpu_shader_create(
    gpu_device* /*gpu*/,
    gpu_shader_type type,
    void* data,
    usize /*size*/,
    const char* /*main_function*/)
{
    // setup shader boilerplate
    const char* prelude =
        "#version 460 core\n"
        "#define VX_VERTEX_SHADER 0\n"
        "#define VX_FRAGMENT_SHADER 1\n"
        "layout(location = " vx_xstr(VX_BASE_INSTANCE_BINDING_SLOT) ") uniform int VX_BASE_INSTANCE;\n"
        "#define VX_INSTANCE_ID (gl_InstanceID + VX_BASE_INSTANCE)\n";
    const char* vertex_prelude = "out gl_PerVertex { vec4 gl_Position; };\n";
    const char* shader_types[] = {
        "#define VX_SHADER VX_VERTEX_SHADER\n",
        "#define VX_SHADER VX_FRAGMENT_SHADER\n",
    };
    const char* prologue = "#line 1\n";
    const char* vertex_sources[] = {
        prelude, vertex_prelude, shader_types[int(type)], prologue, (char*)data};
    const char* other_sources[] = {prelude, shader_types[int(type)], prologue, (char*)data};
    i32 source_count =
        type == gpu_shader_type::vertex ? vx_countof(vertex_sources) : vx_countof(other_sources);
    const char** sources = type == gpu_shader_type::vertex ? vertex_sources : other_sources;

    // compile shader
    gl_shader shader = {};
    shader.object = glCreateShaderProgramv(gpu_convert_enum(type), source_count, (char**)sources);

    // check for errors
    i32 result = 0;
    glGetProgramiv(shader.object, GL_LINK_STATUS, &result);
    if (!result)
    {
        i32 info_log_length;
        glGetProgramiv(shader.object, GL_INFO_LOG_LENGTH, &info_log_length);
        char* buf = (char*)std::malloc(info_log_length);
        glGetProgramInfoLog(shader.object, info_log_length, 0, buf);
        fatal("Shader compilation failed: %s", buf);
    }

    return (gpu_shader*)(*(uptr*)&shader);
}

void gpu_shader_destroy(gpu_device* /*gpu*/, gpu_shader* shader)
{
    if (!shader)
        return;

    gl_shader gl_shader = gpu_convert_handle(shader);
    glDeleteProgram(gl_shader.object);
}

gpu_pipeline* gpu_pipeline_create(
    gpu_device* /*gpu*/,
    gpu_shader* vertex_shader_handle,
    gpu_shader* fragment_shader_handle,
    const gpu_pipeline_options& options)
{
    gl_shader vertex_shader = gpu_convert_handle(vertex_shader_handle);
    gl_shader fragment_shader = gpu_convert_handle(fragment_shader_handle);

    gl_pipeline* pipeline = (gl_pipeline*)std::calloc(1, sizeof gl_pipeline);
    pipeline->blend_enabled = options.blend_enabled;
    pipeline->culling_enabled = options.culling_enabled;
    pipeline->depth_test_enabled = options.depth_test_enabled;
    pipeline->depth_write_enabled = options.depth_write_enabled;
    pipeline->vertex_shader = vertex_shader;
    pipeline->fragment_shader = fragment_shader;
    glCreateProgramPipelines(1, &pipeline->program);

    if (!pipeline->program)
        fatal("Failed to create program pipeline!");

    glUseProgramStages(pipeline->program, GL_VERTEX_SHADER_BIT, vertex_shader.object);
    glUseProgramStages(pipeline->program, GL_FRAGMENT_SHADER_BIT, fragment_shader.object);

    return (gpu_pipeline*)pipeline;
}

void gpu_pipeline_destroy(gpu_device* /*gpu*/, gpu_pipeline* pipeline_handle)
{
    if (!pipeline_handle)
        return;

    gl_pipeline* pipeline = (gl_pipeline*)pipeline_handle;
    glDeleteProgramPipelines(1, &pipeline->program);
    free(pipeline);
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
    if (buffer.target != GL_SHADER_STORAGE_BUFFER)
        fatal("OpenGL backend only accepts SSBO's at this time!");
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, buffer.object);
}

void gpu_channel_set_texture_cmd(gpu_channel* /*channel*/, gpu_texture* texture_handle, u32 index)
{
    gl_texture texture = gpu_convert_handle(texture_handle);
    glBindTextureUnit(index, texture.object);
}

void gpu_channel_set_sampler_cmd(gpu_channel* /*channel*/, gpu_sampler* sampler_handle, u32 index)
{
    gl_sampler sampler = gpu_convert_handle(sampler_handle);
    glBindSamplers(index, 1, &sampler.object);
}

void gpu_channel_set_pipeline_cmd(gpu_channel* channel, gpu_pipeline* pipeline_handle)
{
    gl_device* device = (gl_device*)channel;
    gl_pipeline* pipeline = (gl_pipeline*)pipeline_handle;
    device->current_pipeline = pipeline;

    set_capability(GL_BLEND, pipeline->blend_enabled);
    set_capability(GL_CULL_FACE, pipeline->culling_enabled);
    set_capability(GL_DEPTH_TEST, pipeline->depth_test_enabled);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(pipeline->depth_write_enabled);

    // TODO: these shouldn't be hardcoded here
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindProgramPipeline(pipeline->program);
}

void gpu_channel_set_scissor_cmd(gpu_channel* channel, gpu_scissor_rect* rect)
{
    gl_device* device = (gl_device*)channel;
    // The OpenGL screen coordinates are flipped w.r.t. to the y-axis
    // y-zero is at the top of the screen
    set_capability(GL_SCISSOR_TEST, true);
    i32 fb_height = i32(device->display_size.y * device->display_scale.y);
    glScissor(i32(rect->x), fb_height - i32(rect->y + rect->h), i32(rect->w), i32(rect->h));
}

void gpu_channel_set_viewport_cmd(gpu_channel* /*channel*/, gpu_viewport* viewport)
{
    glViewport(i32(viewport->x), i32(viewport->y), i32(viewport->w), i32(viewport->h));
}

void gpu_channel_draw_primitives_cmd(
    gpu_channel* channel,
    gpu_primitive_type primitive_type,
    u32 vertex_start,
    u32 vertex_count,
    u32 instance_count,
    u32 base_instance)
{
    gl_device* device = (gl_device*)channel;
    glProgramUniform1i(
        device->current_pipeline->vertex_shader.object,
        VX_BASE_INSTANCE_BINDING_SLOT,
        base_instance);
    glDrawArraysInstancedBaseInstance(
        gpu_convert_enum(primitive_type),
        vertex_start,
        vertex_count,
        instance_count,
        base_instance);
}

void gpu_channel_draw_indexed_primitives_cmd(
    gpu_channel* channel,
    gpu_primitive_type primitive_type,
    u32 index_count,
    gpu_index_type index_type,
    gpu_buffer* index_buffer_handle,
    u32 index_byte_offset,
    u32 instance_count,
    i32 base_vertex,
    u32 base_instance)
{
    gl_device* device = (gl_device*)channel;
    gl_buffer index_buffer = gpu_convert_handle(index_buffer_handle);
    if (index_buffer.target != GL_ELEMENT_ARRAY_BUFFER)
        fatal("The index buffer provided was not created as an index buffer!");
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer.object);
    glProgramUniform1i(
        device->current_pipeline->vertex_shader.object,
        VX_BASE_INSTANCE_BINDING_SLOT,
        base_instance);
    glDrawElementsInstancedBaseVertexBaseInstance(
        gpu_convert_enum(primitive_type),
        index_count,
        gpu_convert_enum(index_type),
        (void*)uptr(index_byte_offset),
        instance_count,
        base_vertex,
        base_instance);
}
} // namespace vx
