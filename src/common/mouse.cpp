#include "common/mouse.h"
#include "SDL_events.h"

namespace vx
{
namespace
{
struct mouse_state
{
    int2 current_coordinates{0, 0};
    int2 delta_coordinates{0, 0};
    i32 scroll_delta{0};

    i32 button_state{0};
    i32 button_down_events{0};
    i32 button_up_events{0};

    bool mouse_moved{false};
    bool scroll_moved{false};
};

mouse_state state;
}

/*
 * These enumerations correspond to SDL_BUTTON_* values,
 * defined in http://hg.libsdl.org/SDL/file/default/include/SDL_mouse.h
 */
static_assert(int(button::left) == SDL_BUTTON_LEFT, "button::left not equal to SDL_BUTTON_LEFT");
static_assert(
    int(button::middle) == SDL_BUTTON_MIDDLE,
    "button::middle not equal to SDL_BUTTON_MIDDLE");
static_assert(
    int(button::right) == SDL_BUTTON_RIGHT,
    "Button::right not equal to SDL_BUTTON_RIGHT");

int2 mouse_coordinates() { return state.current_coordinates; }

int2 mouse_delta() { return state.delta_coordinates; }

i32 scroll_delta() { return state.scroll_delta; }

bool mouse_button_down(button b)
{
    return (state.button_down_events & SDL_BUTTON(static_cast<int>(b))) != 0;
}

bool mouse_button_up(button b)
{
    return (state.button_up_events & SDL_BUTTON(static_cast<int>(b))) != 0;
}

bool mouse_button_pressed(button b)
{
    return (state.button_state & SDL_BUTTON(static_cast<int>(b))) != 0;
}

bool scroll_wheel_moved() { return state.scroll_moved; }

bool mouse_moved() { return state.mouse_moved; }

void mouse_update()
{
    state.scroll_delta = 0;
    state.delta_coordinates = int2{0, 0};
    state.button_state =
        SDL_GetMouseState(&state.current_coordinates.x, &state.current_coordinates.y);
    state.button_down_events = 0;
    state.button_up_events = 0;
    state.mouse_moved = false;
    state.scroll_moved = false;
}

void mouse_handle_event(const SDL_Event& event)
{
    switch (event.type)
    {
        case SDL_MOUSEBUTTONDOWN:
            state.button_down_events |= SDL_BUTTON(event.button.button);
            break;

        case SDL_MOUSEBUTTONUP:
            state.button_up_events |= SDL_BUTTON(event.button.button);
            break;

        case SDL_MOUSEWHEEL:
            state.scroll_moved = true;
            state.scroll_delta = event.wheel.y;
            break;

        case SDL_MOUSEMOTION:
            state.mouse_moved = true;
            state.delta_coordinates.x = event.motion.xrel;
            state.delta_coordinates.y = event.motion.yrel;
            break;
    }
}
}
