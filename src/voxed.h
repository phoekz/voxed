#pragma once

#include "common/base.h"
#include "platform/native_platform.h"
#include "platform/gpu.h"

namespace vx
{
struct voxed_cpu_state;
struct voxed_gpu_state;

struct voxed
{
    voxed_cpu_state* cpu;
    voxed_gpu_state* gpu;
};

voxed* voxed_create(platform* platform);

void voxed_process_event(voxed_cpu_state* cpu, const SDL_Event& event);

void voxed_update(voxed_cpu_state* cpu, const platform& platform, float dt);

void voxed_gpu_update(const voxed_cpu_state* cpu, voxed_gpu_state* gpu, gpu_device* device);

void voxed_gui_update(voxed_cpu_state* cpu, const voxed_gpu_state* gpu);

void voxed_gpu_draw(voxed_gpu_state* gpu, gpu_channel* channel);

void voxed_frame_end(voxed* state);

void voxed_quit(voxed* state);
}
