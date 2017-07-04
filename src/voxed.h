#pragma once

#include "platform/native_platform.h"

#include "GL/gl3w.h"

namespace vx
{
struct voxed_state;

voxed_state* voxed_create();

void voxed_update(const platform& platform, voxed_state* state, float dt);

void voxed_render(voxed_state* state);

void voxed_quit(voxed_state* state);
}
