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

namespace vx
{
struct app
{
    bool running = true;

    struct
    {
        SDL_Window* handle;
        int2 size = int2(1600, 1200);
        const char* title = "voxed";
    } window;

    struct
    {
        SDL_GLContext context;
        float4 clearColor = float4(0.95f, 0.95f, 0.95f, 1.0f);
    } gl;

    struct
    {
        float total, delta;
        uint64_t clocks;
    } time;
};
void fatal(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    putc('\n', stderr);
    va_end(args);
    exit(1);
}
}

int main(int /*argc*/, char** /*argv*/)
{
    //
    // init
    //

    vx::app app;

    SDL_Init(SDL_INIT_VIDEO);

    app.window.handle = SDL_CreateWindow(
        app.window.title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        app.window.size.x,
        app.window.size.y,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!app.window.handle)
        vx::fatal("SDL_CreateWindow failed with error: %s", SDL_GetError());

    app.gl.context = SDL_GL_CreateContext(app.window.handle);

    if (gl3wInit() == -1)
        vx::fatal("gl3wInit failed");

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
            vx::float4 c = app.gl.clearColor;
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
