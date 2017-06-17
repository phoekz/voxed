#pragma once

#include "common/aliases.h"
#include "common/platform.h"

#include "SDL.h" // TODO(vinht): Remove once timing, events & inputs have been cleaned away.

namespace vx
{
struct native_window;
struct gpu_device;

struct platform
{
    native_window* window;
    gpu_device* gpu;
};

void platform_init(platform* platform, const char* title, int2 initial_size);
void platform_quit(platform* platform);
void platform_swap_buffers(platform* platform);
}
