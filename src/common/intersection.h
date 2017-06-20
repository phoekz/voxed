#pragma once

#include "geometry.h"
#include "gtx/norm.hpp"

#include <algorithm>
#include <cassert>
#include <limits>

namespace vx
{
static bool ray_intersects_aabb(const ray& r, const bounds3f& b)
{
    float inv_dx = 1.f / r.direction.x;
    float tmin = (b.min.x - r.origin.x) * inv_dx;
    float tmax = (b.max.x - r.origin.x) * inv_dx;

    float inv_dy = 1.f / r.direction.y;
    float tymin = (b.min.y - r.origin.y) * inv_dy;
    float tymax = (b.max.y - r.origin.y) * inv_dy;

    if (tmin > tmax)
        std::swap(tmin, tmax);

    if (tymin > tymax)
        std::swap(tymin, tymax);

    if ((tmin > tymax) || (tymin > tmax))
        return false;

    if (tymin > tmin)
        tmin = tymin;

    if (tymax < tmax)
        tmax = tymax;

    float inv_dz = 1.f / r.direction.z;
    float tzmin = (b.min.z - r.origin.z) * inv_dz;
    float tzmax = (b.max.z - r.origin.z) * inv_dz;

    if (tzmin > tzmax)
        std::swap(tzmin, tzmax);

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;

    return true;
}
}
