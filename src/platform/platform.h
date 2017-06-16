#pragma once

#include "common/aliases.h"

namespace vx
{
struct window;
struct gpu_device;

struct platform
{
    vx::window* window;
    vx::gpu_device* gpu;
};

void platform_init(vx::platform* platform, const char* title, int2 initial_size);
void platform_quit(vx::platform* platform);
void platform_swap_buffers(vx::platform* platform);

struct gpu_channel;

vx::gpu_channel* gpu_channel_open(vx::gpu_device* gpu);
void gpu_channel_close(vx::gpu_device* gpu, vx::gpu_channel* channel);

struct gpu_clear_cmd_args
{
    float4 color;
    float depth;
    u8 stencil;
};

void gpu_channel_clear_cmd(vx::gpu_channel* channel, vx::gpu_clear_cmd_args* args);
}
