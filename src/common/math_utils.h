#pragma once

#include <algorithm>

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
}