#include "common/aliases.h"

#include "SDL.h"
#include "imgui.h"
#include "glm.hpp"
#include "GL/gl3w.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

int main(int /*argc*/, char** /*argv*/)
{
    SDL_Init(SDL_INIT_EVERYTHING);

    printf("Hello, voxed!\n");

    SDL_Quit();

    return 0;
}
