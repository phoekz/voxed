#include "common/aliases.h"

#include "SDL.h"
#include "imgui.h"
#include "glm.hpp"
#include "GL/gl3w.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define vx_countof(arr) (sizeof(arr) / sizeof(arr[0]))
#define vx_fatal(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__), putc('\n', stderr), exit(1)

namespace vx
{
typedef glm::vec2 Vec2f;
typedef glm::vec3 Vec3f;
typedef glm::vec4 Vec4f;

typedef glm::ivec2 Vec2i;
typedef glm::ivec3 Vec3i;
typedef glm::ivec4 Vec4i;

struct App
{
    bool running = true;

    struct
    {
        SDL_Window* handle;
        int width = 1600, height = 1200;
        const char* title = "voxed";
    } window;

    struct
    {
        SDL_GLContext context;
        Vec4f clearColor = Vec4f(0.95f, 0.95f, 0.95f, 1.0f);
    } gl;

    struct
    {
        float total, delta;
        uint64_t clocks;
    } time;
};
}

static vx::App app;

int main(int /*argc*/, char** /*argv*/)
{
    //
    // init
    //

    SDL_Init(SDL_INIT_VIDEO);

    app.window.handle = SDL_CreateWindow(
        app.window.title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        app.window.width,
        app.window.height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!app.window.handle)
        vx_fatal("SDL_CreateWindow failed with error: %s", SDL_GetError());

    app.gl.context = SDL_GL_CreateContext(app.window.handle);

    if (gl3wInit() == -1)
        vx_fatal("gl3wInit failed");

    //
    // main loop
    //

    while (app.running)
    {
        // timing

        {
            uint64_t prev, delta;
            prev = app.time.clocks;
            app.time.clocks = SDL_GetPerformanceCounter();
            delta = app.time.clocks - prev;
            app.time.delta = (float)(delta / (double)SDL_GetPerformanceFrequency());
            app.time.total += app.time.delta;
        }

        // inputs & events

        {
            SDL_Event ev;
            while (SDL_PollEvent(&ev))
            {
                switch (ev.type)
                {
                    case SDL_QUIT:
                        app.running = false;
                }
            }
        }

        // rendering

        {
            vx::Vec4f c = app.gl.clearColor;
            glClearColor(c.x, c.y, c.z, c.w);
            glClear(GL_COLOR_BUFFER_BIT);
        }

        SDL_GL_SwapWindow(app.window.handle);
    }

    //
    // teardown
    //

    SDL_GL_DeleteContext(app.gl.context);
    SDL_DestroyWindow(app.window.handle);
    SDL_Quit();

    return 0;
}
