#pragma once

#include "platform/native_platform.h"

namespace vx
{
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