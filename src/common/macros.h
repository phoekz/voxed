#pragma once

#define vx_countof(arr) (int)(sizeof(arr) / sizeof(arr[0]))
#define vx_xstr(s) vx_str(s)
#define vx_str(s) #s

#define KB *(1024ull)
#define MB *(1024ull * 1024ull)
#define GB *(1024ull * 1024ull * 1024ull)
