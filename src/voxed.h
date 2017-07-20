#pragma once

#include "platform/native_platform.h"

namespace vx
{
struct voxed_state;

enum class voxed_mode
{
    voxel,
    box
};

voxed_state* voxed_create();

void voxed_set_mode(voxed_state* state, voxed_mode mode);

void voxed_update(voxed_state* state, const platform& platform, float dt);

void voxed_render(voxed_state* state);

void voxed_quit(voxed_state* state);
}
