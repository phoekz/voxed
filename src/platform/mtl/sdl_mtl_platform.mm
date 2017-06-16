#include "common/platform.h"

namespace vx
{
void platform_init(vx::platform* platform, const char* title, int2 initial_size) {}

void platform_quit(vx::platform* platform, vx::gpu_device* gpu) {}

void platform_swap_buffers(vx::platform* platform) {}

vx::gpu_channel* gpu_channel_open(vx::gpu_device* gpu) {}

void gpu_channel_close(vx::gpu_device* gpu, vx::gpu_channel* channel) {}

void gpu_channel_clear_cmd(vx::gpu_channel* channel, vx::gpu_clear_cmd_args* args) {}
}
