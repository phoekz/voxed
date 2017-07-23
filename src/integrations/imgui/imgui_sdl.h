#pragma once

#include "imgui.h"
#include "platform/native_platform.h"
#include "platform/gpu.h"

struct SDL_Window;
union SDL_Event;

namespace vx
{
bool imgui_init(platform* platform);
void imgui_shutdown();
void imgui_new_frame(SDL_Window* window);
void imgui_render(gpu_device* gpu, gpu_channel* channel);
bool imgui_process_event(SDL_Event* event);
}
