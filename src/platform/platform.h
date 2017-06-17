#pragma once

#include "common/aliases.h"

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

struct gpu_channel;

gpu_channel* gpu_channel_open(gpu_device* gpu);
void gpu_channel_close(gpu_device* gpu, gpu_channel* channel);

struct gpu_clear_cmd_args
{
    float4 color;
    float depth;
    u8 stencil;
};

void gpu_channel_clear_cmd(gpu_channel* channel, gpu_clear_cmd_args* args);
}
