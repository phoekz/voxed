#pragma once

#include "common/base.h"

namespace vx
{
constexpr float pi = 3.14159265358979323846f;

template<typename T>
T clamp(T t, T min, T max)
{
    return std::max(min, std::min(t, max));
}

template<typename VecType>
VecType remap_range(VecType value, VecType a_min, VecType a_max, VecType b_min, VecType b_max)
{
    return b_min + (value - a_min) * (b_max - b_min) / (a_max - a_min);
}

template<typename T>
constexpr T degrees_to_radians(T degrees)
{
    return (degrees * T(pi)) / T(180);
}

template<typename T>
constexpr T radians_to_degrees(T radians)
{
    return (radians * T(180)) / T(pi);
}

template<typename T>
constexpr T pow2(T x)
{
    return x * x;
}

template<typename T>
constexpr T pow3(T x)
{
    return x * x * x;
}

template<typename T>
constexpr T pows2(T a, T b, T x)
{
    return a + b * x;
}

template<typename T>
constexpr T pows3(T a, T b, T c, T x)
{
    return a + b * x + c * x * x;
}

template<typename T>
VX_FORCE_INLINE T min2(const T& a, const T& b)
{
    return a < b ? a : b;
}

template<typename T>
VX_FORCE_INLINE T max2(const T& a, const T& b)
{
    return a > b ? a : b;
}
}
