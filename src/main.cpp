#include "platform/gpu.h"

#include "imgui.h"
#include "glm.hpp"

#define vx_countof(arr) (sizeof(arr) / sizeof(arr[0]))

namespace vx
{
struct app
{
    bool running = true;

    platform platform;

    struct
    {
        int2 size = int2(1600, 1200);
        const char* title = "voxed";
    } window;

    struct
    {
        float4 bg_color = float4(0.95f, 0.95f, 0.95f, 1.0f);
    } render;

    struct
    {
        float total, delta;
        u64 clocks;
    } time;
};
}

int main(int /*argc*/, char** /*argv*/)
{
    //
    // init
    //

    vx::app app;

    vx::platform_init(&app.platform, app.window.title, app.window.size);

    //
    // main loop
    //

    while (app.running)
    {
        // timing

        {
            vx::u64 prev, delta;
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
            vx::gpu_device* gpu = app.platform.gpu;
            vx::gpu_channel* channel = vx::gpu_channel_open(gpu);
            vx::gpu_clear_cmd_args clear_args = {app.render.bg_color, 0.0f, 0};
            vx::gpu_channel_clear_cmd(channel, &clear_args);
            vx::gpu_channel_close(gpu, channel);
        }

        vx::platform_swap_buffers(&app.platform);
    }

    //
    // teardown
    //

    vx::platform_quit(&app.platform);

    return 0;
}
