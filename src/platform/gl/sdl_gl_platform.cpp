#include "platform/platform.h"
#include "common/error.h"

#include "SDL.h"
#include "GL/gl3w.h"

struct gl_device
{
    SDL_GLContext ctx;
};

namespace vx
{
void platform_init(vx::platform* platform, const char* title, int2 initial_size)
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
        vx::fatal("SDL_CreateWindow failed with error: %s", SDL_GetError());

    sdl_gl_context = SDL_GL_CreateContext(sdl_window);

    if (gl3wInit() == -1)
        vx::fatal("gl3wInit failed");

    platform->window = (vx::window*)sdl_window;
    platform->gpu = (vx::gpu_device*)sdl_gl_context;
}

void platform_quit(vx::platform* platform)
{
    SDL_Window* sdl_window;
    SDL_GLContext sdl_gl_context;

    sdl_window = (SDL_Window*)platform->window;
    sdl_gl_context = (SDL_GLContext)platform->gpu;

    SDL_GL_DeleteContext(sdl_gl_context);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
}

void platform_swap_buffers(vx::platform* platform)
{
    SDL_Window* sdl_window;
    sdl_window = (SDL_Window*)platform->window;
    SDL_GL_SwapWindow(sdl_window);
}

vx::gpu_channel* gpu_channel_open(vx::gpu_device* gpu)
{
    // TODO(vinht): Allocate command buffer.
    (void)gpu;
    return nullptr;
}

void gpu_channel_close(vx::gpu_device* gpu, vx::gpu_channel* channel)
{
    // TODO(vinht): Execute command buffer.
    (void)gpu;
    (void)channel;
}

void gpu_channel_clear_cmd(vx::gpu_channel* channel, vx::gpu_clear_cmd_args* args)
{
    // TODO(vinht): Record to a command buffer.
    (void)channel;

    float4 c = args->color;
    glClearColor(c.x, c.y, c.z, c.w);
    glClear(GL_COLOR_BUFFER_BIT);
}
}