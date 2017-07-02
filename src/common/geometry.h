#pragma once

#include "aliases.h"

namespace vx
{

enum axis
{
    axis_x,
    axis_y,
    axis_z,
    axis_count,
};

enum signed_axis
{
    signed_axis_neg_x,
    signed_axis_pos_x,
    signed_axis_neg_y,
    signed_axis_pos_y,
    signed_axis_neg_z,
    signed_axis_pos_z,
    signed_axis_count,
};

enum axis_plane
{
    axis_plane_xy,
    axis_plane_yz,
    axis_plane_zx,
    axis_plane_count,
};

inline float3 axis_plane_normal(axis_plane p)
{
    assert(p >= axis_plane_xy);
    assert(p <= axis_plane_zx);
    float3 n;
    n[(p + 2) % 3] = 1.0f;
    return n;
}

struct ray
{
    float3 origin;
    float3 direction;
};

struct plane
{
    plane(const float3& p0, const float3& p1, const float3& p2)
        : point(p0), normal(glm::normalize(glm::cross(p1 - p0, p2 - p0)))
    {
    }

    plane(const float3& p, const float3& n) : point(p), normal(n) {}

    float3 point;
    float3 normal;
};

template<typename VecType>
struct bounds
{
    using value_type = typename VecType::value_type;

    VecType min;
    VecType max;
};

template<typename VecType>
bool operator==(const bounds<VecType>& lhs, const bounds<VecType>& rhs)
{
    return lhs.min == rhs.min && lhs.max == rhs.max;
}

template<typename VecType>
bool operator!=(const bounds<VecType>& lhs, const bounds<VecType>& rhs)
{
    return lhs.min != rhs.min && lhs.max != rhs.max;
}

template<typename VecType>
VecType center(const bounds<VecType>& b)
{
    return typename VecType::value_type(0.5) * (b.min + b.max);
}

template<typename VecType>
VecType extents(const bounds<VecType>& b)
{
    return b.max - b.min;
}

using bounds2f = bounds<float2>;
using bounds2i = bounds<int2>;
using bounds3f = bounds<float3>;
using bounds3i = bounds<int3>;
}
