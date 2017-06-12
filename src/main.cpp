#include "SDL.h"
#include <stdio.h>

int main(int /*argc*/, char** /*argv*/)
{
    SDL_Init(SDL_INIT_EVERYTHING);

    printf("Hello, voxed!\n");

    SDL_Quit();

    return 0;
}
