#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

namespace vx
{
static void fatal(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    putc('\n', stderr);
    va_end(args);
    exit(1);
}
}
