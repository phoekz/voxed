#pragma once

#define VX_UNKNOWN 0

#define VX_LINUX 1
#define VX_WINDOWS 2
#define VX_MACOSX 3

#define VX_POSIX 4
#define VX_WIN32 5

#if defined(__linux__)
    #define VX_OS VX_LINUX
    #define VX_OS_STRING "Linux"
    #define VX_PLATFORM VX_POSIX
#elif defined(_WIN32) || defined(_WIN64)
    #define VX_OS VX_WINDOWS
    #define VX_OS_STRING "Windows"
    #define VX_PLATFORM VX_WIN32
#elif defined(__APPLE__) || defined(MACOSX)
    #define VX_OS VX_MACOSX
    #define VX_OS_STRING "OSX"
    #define VX_PLATFORM VX_POSIX
#else
    #define VX_OS VX_UNKNOWN
    #define VX_OS_STRING "Unknown"
    #define VX_PLATFORM VX_UNKNOWN
#endif

static_assert(VX_OS != VX_UNKNOWN, "Platform detection failed");
