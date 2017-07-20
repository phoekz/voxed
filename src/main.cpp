#include "voxed.h"
#include "common/error.h"
#include "common/mouse.h"
#include "common/macros.h"

#include "integrations/imgui/imgui_sdl.h"
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"

#include <algorithm>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>

// lol, windows
#undef near
#undef far

namespace vx
{
namespace
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
}

int main(int /*argc*/, char** /*argv*/)
{
    //
    // init
    //

    vx::app app;
    vx::voxed_state* voxed;

    vx::platform_init(&app.platform, app.window.title, app.window.size);

    if (!vx::imgui_init(&app.platform))
        vx::fatal("ImGui initialization failed");

    voxed = vx::voxed_create();
    if (!voxed)
        vx::fatal("Voxel app initialization failed");

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
            vx::mouse_update();

            SDL_Event ev;
            while (SDL_PollEvent(&ev))
            {
                switch (ev.type)
                {
                    case SDL_QUIT:
                        app.running = false;
                }

                vx::mouse_handle_event(ev);
                vx::imgui_process_event(&ev);
            }
        }

        // app

        {
            vx::voxed_update(voxed, app.platform, app.time.delta);
        }

        // gui

        {
            vx::imgui_new_frame(app.platform.window);
            ImGui::Begin("Hello, ImGui");
            static float greatness;
            ImGui::SliderFloat("Greatness", &greatness, 0.0f, 1.0f);
            if (ImGui::Button("Yay!"))
                printf("%f\n", greatness);
            ImGui::End();
        }

        // rendering

        vx::platform_frame_begin(&app.platform);

        {
            vx::gpu_device* gpu = app.platform.gpu;
            vx::gpu_channel* channel = vx::gpu_channel_open(gpu);
            vx::gpu_clear_cmd_args clear_args = {app.render.bg_color, 1.0f, 0};
            vx::voxed_render(voxed);
            vx::imgui_render(&app.platform, channel);
            vx::gpu_channel_close(gpu, channel);
        }

        vx::platform_frame_end(&app.platform);
    }

    //
    // teardown
    //

    vx::voxed_quit(voxed);
    vx::imgui_shutdown();
    vx::platform_quit(&app.platform);

    return 0;
}
