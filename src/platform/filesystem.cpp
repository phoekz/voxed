#include "platform/filesystem.h"

namespace vx
{
char* read_whole_file(const char* path, usize* out_file_size)
{
    FILE* f;
    usize fsize;
    char* buf;

    f = std::fopen(path, "rb");

    if (!f)
        fatal("Could not open file at %s", path);

    std::fseek(f, 0, SEEK_END);
    fsize = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    buf = (char*)malloc(fsize + 1);
    fread(buf, fsize, 1, f);
    buf[fsize] = 0;
    std::fclose(f);

    if (out_file_size)
        *out_file_size = fsize;

    return buf;
}
}
