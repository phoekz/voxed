#pragma once

#include <cstdio>
#include <cstdarg>
#include <cstdlib>

namespace vx
{
inline void fatal(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::vfprintf(stderr, fmt, args);
    std::putc('\n', stderr);
    va_end(args);
    std::exit(1);
}
}
