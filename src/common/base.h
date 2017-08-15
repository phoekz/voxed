#pragma once

//
// libc
//

#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

//
// STL
//

#include <algorithm>

//
// 3rd party
//

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtx/norm.hpp>

//
// platform
//

#define VX_OS_UNKNOWN 0
#define VX_OS_LINUX 1
#define VX_OS_WINDOWS 2
#define VX_OS_MACOSX 3

#define VX_PLATFORM_UNKNOWN 0
#define VX_PLATFORM_WIN32 1
#define VX_PLATFORM_POSIX 2

#define VX_GRAPHICS_API_UNKNOWN 0
#define VX_GRAPHICS_API_METAL 1
#define VX_GRAPHICS_API_OPENGL 2

#if defined(__linux__)
#define VX_OS VX_OS_LINUX
#define VX_OS_STRING "Linux"
#define VX_PLATFORM VX_PLATFORM_POSIX
#define VX_GRAPHICS_API VX_GRAPHICS_API_OPENGL
#elif defined(_WIN32) || defined(_WIN64)
#define VX_OS VX_OS_WINDOWS
#define VX_OS_STRING "Windows"
#define VX_PLATFORM VX_PLATFORM_WIN32
#define VX_GRAPHICS_API VX_GRAPHICS_API_OPENGL
#elif defined(__APPLE__) || defined(MACOSX)
#define VX_OS VX_OS_MACOSX
#define VX_OS_STRING "OSX"
#define VX_PLATFORM VX_PLATFORM_POSIX
#define VX_GRAPHICS_API VX_GRAPHICS_API_METAL
#else
#define VX_OS VX_OS_UNKNOWN
#define VX_OS_STRING "Unknown"
#define VX_PLATFORM VX_PLATFORM_UNKNOWN
#define VX_GRAPHICS_API VX_GRAPHICS_API_UNKNOWN
#endif

static_assert(VX_OS != VX_OS_UNKNOWN, "Platform detection failed");

//
// attributes
//

#if VX_PLATFORM == VX_PLATFORM_WIN32
#define VX_NO_RETURN __declspec(noreturn)
#define VX_FORCE_INLINE __forceinline
#elif VX_PLATFORM == VX_PLATFORM_POSIX
#define VX_NO_RETURN _Noreturn
#define VX_FORCE_INLINE __attribute__((always_inline))
#endif

//
// intrinsics
//

#if VX_PLATFORM == VX_PLATFORM_WIN32
#define vx_popcnt(x) __popcnt(x)
#elif VX_PLATFORM == VX_PLATFORM_POSIX
#define vx_popcnt(x) __builtin_popcount(x)
#endif

//
// macros
//

#define vx_countof(arr) (int)(sizeof(arr) / sizeof(arr[0]))
#define vx_xstr(s) vx_str(s)
#define vx_str(s) #s

//
// aliases
//

namespace vx
{
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using usize = std::size_t;
using iptr = std::intptr_t;
using uptr = std::uintptr_t;
using ptrdiff = std::ptrdiff_t;

using float2 = glm::vec2;
using float3 = glm::vec3;
using float4 = glm::vec4;

using int2 = glm::ivec2;
using int3 = glm::ivec3;
using int4 = glm::ivec4;

using uint2 = glm::uvec2;
using uint3 = glm::uvec3;
using uint4 = glm::uvec4;

using float2x2 = glm::mat2;
using float3x3 = glm::mat3;
using float4x4 = glm::mat4;
}

//
// units
//

#define KB *(1024ull)
#define MB *(1024ull * 1024ull)
#define GB *(1024ull * 1024ull * 1024ull)

//
// error handling
//

namespace vx
{
VX_NO_RETURN inline void fatal(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::vfprintf(stderr, fmt, args);
    std::putc('\n', stderr);
    va_end(args);
    std::exit(1);
}
}
