#pragma once

#include "aliases.h"

namespace vx
{

struct ray
{
    float3 origin;
    float3 direction;
    float t;
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
