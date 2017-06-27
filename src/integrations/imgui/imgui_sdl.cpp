#include "imgui_sdl.h"
#include "common/aliases.h"
#include "common/error.h"
#include "common/macros.h"
#include "platform/gpu.h"
#include "platform/filesystem.h"

#include "SDL.h"
#include "SDL_syswm.h"

namespace vx
{
namespace
{
const char* imgui_get_clipboard(void*) { return SDL_GetClipboardText(); }

void imgui_set_clipboard(void*, const char* text) { SDL_SetClipboardText(text); }

struct
{
    double time;
    bool mouse_pressed[3];
    float mouse_wheel;

    usize vertex_buffer_capacity = 64 KB;
    usize index_buffer_capacity = 16 KB;

    gpu_buffer* vertex_buffer;
    gpu_buffer* index_buffer;
    gpu_buffer* constants_buffer;
    gpu_vertex_desc* vertex_desc;
    gpu_texture* font_texture;
    gpu_sampler* font_sampler;

    gpu_shader* vertex_shader;
    gpu_shader* fragment_shader;
    gpu_pipeline* pipeline;
} imgui_ctx;

struct imgui_userdata
{
    platform* platform;
    gpu_channel* channel;
};

void imgui_render_draw_lists(ImDrawData* draw_data)
{
    ImGuiIO& io = ImGui::GetIO();
    imgui_userdata* user = (imgui_userdata*)io.UserData;
    platform* platform = user->platform;
    gpu_device* gpu = platform->gpu;
    gpu_channel* channel = user->channel;

    //
    // Display scaling
    //

    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0)
        return;

    // TODO(vinht): With this enabled, scissor rect is scaled incorrectly
    // under Metal backend. Figure out what the GL version does differently.

    // draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    //
    // Global state
    //

    {
        const float ortho_projection[4][4] = {
            {2.0f / io.DisplaySize.x, 0.0f, 0.0f, 0.0f},
            {0.0f, 2.0f / -io.DisplaySize.y, 0.0f, 0.0f},
            {0.0f, 0.0f, -1.0f, 0.0f},
            {-1.0f, 1.0f, 0.0f, 1.0f},
        };

        gpu_buffer* constants_buffer = imgui_ctx.constants_buffer;
        gpu_buffer_update(
            gpu, constants_buffer, (void*)ortho_projection, sizeof(ortho_projection), 0);

        gpu_channel_set_pipeline_cmd(channel, imgui_ctx.pipeline);
        gpu_channel_set_texture_cmd(channel, imgui_ctx.font_texture, 0);
        gpu_channel_set_sampler_cmd(channel, imgui_ctx.font_sampler, 0);
    }

    //
    // Execute command lists
    //

    for (int list_index = 0; list_index < draw_data->CmdListsCount; ++list_index)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[list_index];
        u32 index_offset = 0;

        // Update buffers

        usize vertex_data_size = cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
        usize index_data_size = cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx);

        void* vertex_data = cmd_list->VtxBuffer.Data;
        void* index_data = cmd_list->IdxBuffer.Data;

        if (vertex_data_size > imgui_ctx.vertex_buffer_capacity ||
            index_data_size > imgui_ctx.index_buffer_capacity)
            fatal("TODO!");

        gpu_buffer* vertex_buffer = imgui_ctx.vertex_buffer;
        gpu_buffer* index_buffer = imgui_ctx.index_buffer;
        gpu_buffer* constants_buffer = imgui_ctx.constants_buffer;

        gpu_buffer_update(gpu, vertex_buffer, vertex_data, vertex_data_size, 0);
        gpu_buffer_update(gpu, index_buffer, index_data, index_data_size, 0);

        gpu_channel_set_vertex_desc_cmd(channel, imgui_ctx.vertex_desc);
        gpu_channel_set_buffer_cmd(channel, vertex_buffer, 0);
        gpu_channel_set_buffer_cmd(channel, constants_buffer, 1);

        for (int cmd_index = 0; cmd_index < cmd_list->CmdBuffer.Size; ++cmd_index)
        {
            const ImDrawCmd* draw_cmd = &cmd_list->CmdBuffer[cmd_index];

            if (draw_cmd->UserCallback)
                draw_cmd->UserCallback(cmd_list, draw_cmd);
            else
            {
                // TODO(vinht): Bind draw_cmd->TextureId.

                u32 sx0, sy0, sx1, sy1;
                sx0 = (u32)draw_cmd->ClipRect.x;
                sy0 = (u32)draw_cmd->ClipRect.y;
                sx1 = (u32)draw_cmd->ClipRect.z;
                sy1 = (u32)draw_cmd->ClipRect.w;
                gpu_scissor_rect scissor_rect{sx0, fb_height - sy1, sx1 - sx0, sy1 - sy0};
                gpu_channel_set_scissor_cmd(channel, &scissor_rect);

                gpu_channel_draw_indexed_primitives_cmd(
                    channel,
                    gpu_primitive_type::triangle,
                    draw_cmd->ElemCount,
                    gpu_index_type::u16,
                    index_buffer,
                    index_offset * sizeof(u16));
            }

            index_offset += draw_cmd->ElemCount;
        }
    }
}
}

bool imgui_init(platform* platform)
{
    SDL_Window* window = platform->window;
    gpu_device* gpu = platform->gpu;
    ImGuiIO& io = ImGui::GetIO();

    //
    // Key bindings
    //

    {
        io.KeyMap[ImGuiKey_Tab] = SDLK_TAB;
        io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
        io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
        io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
        io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
        io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
        io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
        io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
        io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
        io.KeyMap[ImGuiKey_Delete] = SDLK_DELETE;
        io.KeyMap[ImGuiKey_Backspace] = SDLK_BACKSPACE;
        io.KeyMap[ImGuiKey_Enter] = SDLK_RETURN;
        io.KeyMap[ImGuiKey_Escape] = SDLK_ESCAPE;
        io.KeyMap[ImGuiKey_A] = SDLK_a;
        io.KeyMap[ImGuiKey_C] = SDLK_c;
        io.KeyMap[ImGuiKey_V] = SDLK_v;
        io.KeyMap[ImGuiKey_X] = SDLK_x;
        io.KeyMap[ImGuiKey_Y] = SDLK_y;
        io.KeyMap[ImGuiKey_Z] = SDLK_z;

        io.RenderDrawListsFn = imgui_render_draw_lists;
        io.SetClipboardTextFn = imgui_set_clipboard;
        io.GetClipboardTextFn = imgui_get_clipboard;
        io.ClipboardUserData = nullptr;
    }

    //
    // Native window handle
    //

    {
#ifdef _WIN32
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(window, &wmInfo);
        io.ImeWindowHandle = wmInfo.info.win.window;
#else
        (void)window;
#endif
    }

    //
    // Shaders & pipeline
    //

    {
        char* program_src;
        usize program_size;

#if VX_GRAPHICS_API == VX_METAL
        // TODO: copy metal shader resources
        program_src = read_whole_file("src/shaders/mtl/gui.metallib", &program_size);
#elif VX_GRAPHICS_API == VX_OPENGL
        program_src = read_whole_file("shaders/gl/gui.glsl", &program_size);
#endif

        imgui_ctx.vertex_shader = gpu_shader_create(
            gpu, gpu_shader_type::vertex, program_src, program_size, "vertex_main");

        imgui_ctx.fragment_shader = gpu_shader_create(
            gpu, gpu_shader_type::fragment, program_src, program_size, "fragment_main");

        free(program_src);
    }

    //
    // Pipeline
    //

    {
        gpu_pipeline_options opts = {0};
        opts.blend_enabled = true;
        opts.culling_enabled = false;
        opts.depth_test_enabled = false;
        opts.scissor_test_enabled = true;

        imgui_ctx.pipeline =
            gpu_pipeline_create(gpu, imgui_ctx.vertex_shader, imgui_ctx.fragment_shader, opts);
    }

    //
    // Buffers
    //

    {
        imgui_ctx.vertex_buffer =
            gpu_buffer_create(gpu, imgui_ctx.vertex_buffer_capacity, gpu_buffer_type::vertex);
        imgui_ctx.index_buffer =
            gpu_buffer_create(gpu, imgui_ctx.index_buffer_capacity, gpu_buffer_type::index);
        imgui_ctx.constants_buffer =
            gpu_buffer_create(gpu, sizeof(float4x4), gpu_buffer_type::constant);
    }

    //
    // Vertex Descriptor
    //

    {
        gpu_vertex_desc_attribute attributes[3];
        usize pos_size, uv_size, col_size;

        pos_size = sizeof(float2);
        uv_size = sizeof(float2);
        col_size = 4u;

        attributes[0].format = gpu_vertex_format::float2;
        attributes[0].buffer_index = 0;
        attributes[0].offset = 0;

        attributes[1].format = gpu_vertex_format::float2;
        attributes[1].buffer_index = 1;
        attributes[1].offset = pos_size;

        attributes[2].format = gpu_vertex_format::rgba8_unorm;
        attributes[2].buffer_index = 2;
        attributes[2].offset = pos_size + uv_size;

        imgui_ctx.vertex_desc = gpu_vertex_desc_create(
            gpu, attributes, vx_countof(attributes), (u32)(pos_size + uv_size + col_size));
    }

    //
    // Textures
    //

    {
        u8* pixels;
        int w, h;

        io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);

        imgui_ctx.font_texture =
            gpu_texture_create(gpu, w, h, gpu_pixel_format::rgba8_unorm, pixels);

        io.Fonts->TexID = (void*)(iptr)imgui_ctx.font_texture;
    }

    //
    // Samplers
    //

    {
        imgui_ctx.font_sampler = gpu_sampler_create(
            gpu, gpu_filter_mode::linear, gpu_filter_mode::linear, gpu_filter_mode::nearest);
    }

    return true;
}

void imgui_shutdown()
{
    // TODO(vinht): Free GPU resources.
    ImGui::Shutdown();
}

void imgui_new_frame(SDL_Window* window)
{
    ImGuiIO& io = ImGui::GetIO();

    //
    // Display size
    //

    {
        int w, h;
        int display_w, display_h;
        SDL_GetWindowSize(window, &w, &h);
        SDL_GL_GetDrawableSize(window, &display_w, &display_h);
        io.DisplaySize = ImVec2((float)w, (float)h);
        io.DisplayFramebufferScale =
            ImVec2(w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0);
    }

    //
    // Timing
    //

    {
        u32 time = SDL_GetTicks();
        double current_time = time / 1000.0;
        io.DeltaTime =
            imgui_ctx.time > 0.0 ? (float)(current_time - imgui_ctx.time) : (float)(1.0f / 60.0f);
        imgui_ctx.time = current_time;
    }

    //
    // Inputs
    //

    {
        int mx, my;
        u32 mouse_mask = SDL_GetMouseState(&mx, &my);
        bool mouse_pressed[3];
        mouse_pressed[0] = (mouse_mask & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
        mouse_pressed[1] = (mouse_mask & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
        mouse_pressed[2] = (mouse_mask & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;

        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MOUSE_FOCUS)
            io.MousePos = ImVec2((float)mx, (float)my);
        else
            io.MousePos = ImVec2(-1, -1);

        io.MouseDown[0] = imgui_ctx.mouse_pressed[0] || mouse_pressed[0];
        io.MouseDown[1] = imgui_ctx.mouse_pressed[1] || mouse_pressed[1];
        io.MouseDown[2] = imgui_ctx.mouse_pressed[2] || mouse_pressed[2];
        imgui_ctx.mouse_pressed[0] = false;
        imgui_ctx.mouse_pressed[1] = false;
        imgui_ctx.mouse_pressed[2] = false;

        io.MouseWheel = imgui_ctx.mouse_wheel;
        imgui_ctx.mouse_wheel = 0.0f;

        SDL_ShowCursor(io.MouseDrawCursor ? 0 : 1);
    }

    //
    // Start the frame
    //

    ImGui::NewFrame();
}

void imgui_render(platform* platform, gpu_channel* channel)
{
    imgui_userdata user{platform, channel};
    ImGui::GetIO().UserData = &user;
    ImGui::Render();
}

bool imgui_process_event(SDL_Event* event)
{
    ImGuiIO& io = ImGui::GetIO();

    switch (event->type)
    {
        case SDL_MOUSEWHEEL:
        {
            if (event->wheel.y > 0)
                imgui_ctx.mouse_wheel = 1;
            if (event->wheel.y < 0)
                imgui_ctx.mouse_wheel = -1;
            return true;
        }
        case SDL_MOUSEBUTTONDOWN:
        {
            if (event->button.button == SDL_BUTTON_LEFT)
                imgui_ctx.mouse_pressed[0] = true;
            if (event->button.button == SDL_BUTTON_RIGHT)
                imgui_ctx.mouse_pressed[1] = true;
            if (event->button.button == SDL_BUTTON_MIDDLE)
                imgui_ctx.mouse_pressed[2] = true;
            return true;
        }
        case SDL_TEXTINPUT:
        {
            io.AddInputCharactersUTF8(event->text.text);
            return true;
        }
        case SDL_KEYDOWN:
        case SDL_KEYUP:
        {
            int key = event->key.keysym.sym & ~SDLK_SCANCODE_MASK;
            io.KeysDown[key] = (event->type == SDL_KEYDOWN);
            io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
            io.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
            io.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
            io.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);
            return true;
        }
    }
    return false;
}
}
