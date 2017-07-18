#pragma once

#include "geometry.h"

namespace vx
{
bool ray_intersects_aabb(const ray& r, const bounds3f& b, float* out_t);

void ray_intersects_plane(const ray& r, const plane& plane, float& out_t);
}
