#pragma once

#include "aliases.h"

union SDL_Event;

namespace vx
{
/*
 * These enumerations correspond to SDL_BUTTON_* values,
 * defined in http://hg.libsdl.org/SDL/file/default/include/SDL_mouse.h
 */
enum class button : int
{
    left = 1u,
    middle,
    right
};

int2 mouse_coordinates();

int2 mouse_delta();

i32 scroll_delta();

// did the specified mouse button go down this frame?
bool mouse_button_down(button);

// did the specified mouse button go up this frame?
bool mouse_button_up(button);

// was the specified mouse button pressed this frame?
bool mouse_button_pressed(button);

// did the scroll wheel move this frame?
bool scroll_wheel_moved();

// did the mouse move this frame?
bool mouse_moved();

// handle an sdl event
void mouse_handle_event(const SDL_Event&);

// update the mouse state for the current frame
void mouse_update();
}
