#include "voxed.h"

#include "common/mouse.h"
#include "integrations/imgui/imgui_sdl.h"

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

    voxed = vx::voxed_create(&app.platform);
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
            vx::voxed_gui(voxed);
        }

        // rendering

        vx::platform_frame_begin(&app.platform);

        {
            vx::gpu_device* gpu = app.platform.gpu;
            vx::voxed_gpu_update(voxed, gpu);

            vx::gpu_channel* channel = vx::gpu_channel_open(gpu);
            vx::gpu_clear_cmd_args clear_args{app.render.bg_color, 1.0f, 0};
            vx::gpu_channel_clear_cmd(channel, &clear_args);
            vx::voxed_gpu_draw(voxed, channel);
            vx::imgui_render(gpu, channel);
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
