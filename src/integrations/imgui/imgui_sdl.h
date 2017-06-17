// ImGui SDL2 binding with OpenGL3
// In this binding, ImTextureID is used to store an OpenGL 'GLuint' texture identifier. Read the FAQ
// about ImTextureID in imgui.cpp.

// Adapted from
// https://github.com/ocornut/imgui

struct SDL_Window;
union SDL_Event;

namespace vx
{
bool imgui_init(SDL_Window* window);
void imgui_shutdown();
void imgui_new_frame(SDL_Window* window);
bool imgui_process_event(SDL_Event* event);

// Use if you want to reset your rendering device without losing ImGui state.
void imgui_invalidate_device_objects();
bool imgui_create_device_objects();
}
