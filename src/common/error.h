#pragma once

#include <cstdio>
#include <cstdarg>
#include <cstdlib>

#include "common/platform.h"
#if VX_PLATFORM == VX_WIN32
#define VX_NO_RETURN __declspec(noreturn)
#elif VX_PLATFORM == VX_POSIX
#define VX_NO_RETURN _Noreturn
#endif

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
