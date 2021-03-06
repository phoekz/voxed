#pragma once

#include "common/base.h"

#include "SDL.h" // TODO(vinht): Remove once timing, events & inputs have been cleaned away.

namespace vx
{
struct gpu_device;

struct platform
{
    SDL_Window* window;
    gpu_device* gpu;
};

void platform_init(platform* platform, const char* title, int2 initial_size);
void platform_quit(platform* platform);
void platform_frame_begin(platform* platform);
void platform_frame_end(platform* platform);
}
