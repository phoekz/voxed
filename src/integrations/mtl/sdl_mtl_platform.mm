#include "platform/gpu.h"

namespace vx
{
void platform_init(platform* /*platform*/, const char* /*title*/, int2 /*initial_size*/) {}

void platform_quit(platform* /*platform*/, gpu_device* /*gpu*/) {}

void platform_swap_buffers(platform* /*platform*/) {}

gpu_channel* gpu_channel_open(gpu_device* /*gpu*/) {}

void gpu_channel_close(gpu_device* /*gpu*/, gpu_channel* /*channel*/) {}

void gpu_channel_clear_cmd(gpu_channel* channel, gpu_clear_cmd_args* args) {}
}
