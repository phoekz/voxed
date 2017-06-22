#include "common/error.h"
#include "common/geometry.h"
#include "common/intersection.h"
#include "common/mouse.h"
#include "common/macros.h"
#include "platform/gpu.h"
#include "platform/filesystem.h"

#include "integrations/imgui/imgui_sdl.h"
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "GL/gl3w.h"

#include <algorithm>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>

// lol, windows
#undef near
#undef far

namespace vx
{
namespace
{
float clamp(float t, float min, float max) { return std::max(min, std::min(t, max)); }

constexpr float degrees_to_radians(float degrees) { return (degrees * float(M_PI)) / 180.f; }

constexpr float radians_to_degrees(float radians) { return (radians * 180.f) / float(M_PI); }

struct app
{
    bool running = true;

    platform platform;

    struct
    {
        int2 size = int2(640, 640);
        const char* title = "voxed";
    } window;

    struct
    {
        float4 bg_color = float4(0.95f, 0.95f, 0.95f, 1.0f);
    } render;

    struct
    {
        float total, delta;
        u64 clocks;
    } time;
};

//
// prototype app
//

struct orbit_camera
{
    float fovy{degrees_to_radians(60.f)};
    float near{0.01f};
    float far{100.f};

    float3 focal_point{0.f, 0.f, 0.f};
    float polar{float(0.f)};
    float azimuth{0.f};
    float radius{5.f};
    const float min_radius{1.5f}, max_radius{10.f};
};

#define VX_GRID_SIZE 16

struct voxel_app
{
    GLuint cube_buffer{0u};
    GLuint cube_array{0u};
    GLuint shader{0u};
    float3 cube_color{0.1f, 0.1f, 0.1f};
    float3 selection_color{1.f, 0.2f, 0.2f};
    float4x4 camera_view{};
    orbit_camera camera{};
    bounds3f scene_bounds{float3{-1.f}, float3{1.f}};
    u8 voxel_grid[VX_GRID_SIZE * VX_GRID_SIZE * VX_GRID_SIZE];
};

void on_camera_dolly(orbit_camera& camera, float dz, float dt)
{
    const float scale = 8.f;
    // the delta radius is scaled here by the radius itself, so that the scaling acts faster
    // the further away the camera is
    camera.radius -= dt * scale * dz * camera.radius;
    camera.radius = clamp(camera.radius, camera.min_radius, camera.max_radius);
}

void on_camera_orbit(orbit_camera& camera, float dx, float dy, float dt)
{
    const float scale = 16.f;
    camera.polar += dt * scale * degrees_to_radians(dy);
    camera.azimuth -= dt * scale * degrees_to_radians(dx);
    camera.polar = clamp(camera.polar, -float(0.499 * M_PI), float(0.499 * M_PI));
}

// The coordinate system used here is the following.
// The polar is the angle between the z-axis and the y-axis
// The azimuth is the angle between the z-axis and the x-axis.
float3 forward(const orbit_camera& camera)
{
    const float cos_polar = std::cos(camera.polar);
    return glm::normalize(float3(
        -cos_polar * std::sin(camera.azimuth),
        -std::sin(camera.polar),
        -cos_polar * std::cos(camera.azimuth)));
}

float3 right(const orbit_camera& camera)
{
    const float cos_polar = std::cos(camera.polar);
    float z = cos_polar * std::cos(camera.azimuth);
    float x = cos_polar * std::sin(camera.azimuth);
    return glm::normalize(float3(z, 0.f, -x));
}

inline float3 up(const orbit_camera& camera) { return glm::cross(right(camera), forward(camera)); }

inline float3 eye(const orbit_camera& camera)
{
    return camera.focal_point - camera.radius * forward(camera);
}

ray generate_camera_ray(const app& app, const orbit_camera& camera)
{
    ray ray;
    ray.origin = eye(camera);

    int w, h;
    SDL_GetWindowSize(app.platform.window, &w, &h);
    const int2 coordinates = mouse_coordinates();
    const float aspect = float(w) / h;
    const float nx = 2.f * (float(coordinates.x) / w - .5f);
    const float ny = -2.f * (float(coordinates.y) / h - .5f);

    float vertical_extent = std::tan(0.5f * camera.fovy) * camera.near;
    ray.direction = forward(camera) * camera.near;
    ray.direction += aspect * nx * vertical_extent * right(camera);
    ray.direction += ny * vertical_extent * up(camera);
    ray.direction = glm::normalize(ray.direction);

    return ray;
}

GLuint compile_gl_shader_from_file(const char* file_path)
{
    GLint status;
    char* shader_source = read_whole_file(file_path, nullptr);
    GLuint vs_stage = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs_stage = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint program = glCreateProgram();
    char* vs_defs[] = {
        "#version 450 core\n", "#define VX_SHADER 0\n", shader_source,
    };
    char* fs_defs[] = {
        "#version 450 core\n", "#define VX_SHADER 1\n", shader_source,
    };

    glShaderSource(vs_stage, vx_countof(vs_defs), vs_defs, 0);
    glShaderSource(fs_stage, vx_countof(fs_defs), fs_defs, 0);

    glCompileShader(vs_stage);
    glCompileShader(fs_stage);

    glGetShaderiv(vs_stage, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        char buf[2048];
        glGetShaderInfoLog(vs_stage, sizeof(buf) - 1, 0, buf);
        fatal("Vertex shader compilation failed (%s)\n%s\n", file_path, buf);
    }

    glGetShaderiv(fs_stage, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        char buf[2048];
        glGetShaderInfoLog(fs_stage, sizeof(buf) - 1, 0, buf);
        fatal("Fragment shader compilation failed (%s)\n%s\n", file_path, buf);
    }

    glAttachShader(program, vs_stage);
    glAttachShader(program, fs_stage);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status)
    {
        char buf[2048];
        glGetProgramInfoLog(program, sizeof(buf) - 1, 0, buf);
        fatal("Program linking failed (%s)\n%s\n", file_path, buf);
    }

    free(shader_source);
    return program;
}

bool voxel_app_initialize(voxel_app& vox_app)
{
    vox_app.shader = compile_gl_shader_from_file("src/shaders/gl/line.glsl");
    memset(vox_app.voxel_grid, 0, sizeof(vox_app.voxel_grid));

    for (int z = 0; z < VX_GRID_SIZE; z++)
        for (int y = 0; y < VX_GRID_SIZE; y++)
            for (int x = 0; x < VX_GRID_SIZE; x++)
                if (rand() % 24 == 0)
                {
                    int i = x + y * VX_GRID_SIZE + z * VX_GRID_SIZE * VX_GRID_SIZE;
                    vox_app.voxel_grid[i] = 1;
                }

    glGenVertexArrays(1, &vox_app.cube_array);
    glBindVertexArray(vox_app.cube_array);
    {
        glGenBuffers(1, &vox_app.cube_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, vox_app.cube_buffer);

        // the min and max corners of the cube to render
        float3 mn = vox_app.scene_bounds.min;
        float3 mx = vox_app.scene_bounds.max;

        float3 attribs[] = {
            mn,
            {mn.x, mn.y, mx.z},
            mn,
            {mn.x, mx.y, mn.z},
            mn,
            {mx.x, mn.y, mn.z},

            mx,
            {mx.x, mx.y, mn.z},
            mx,
            {mx.x, mn.y, mx.z},
            mx,
            {mn.x, mx.y, mx.z},

            {mn.x, mx.y, mn.z},
            {mn.x, mx.y, mx.z},
            {mn.x, mx.y, mn.z},
            {mx.x, mx.y, mn.z},
            {mx.x, mn.y, mn.z},
            {mx.x, mx.y, mn.z},

            {mx.x, mn.y, mx.z},
            {mn.x, mn.y, mx.z},
            {mx.x, mn.y, mx.z},
            {mx.x, mn.y, mn.z},
            {mn.x, mx.y, mx.z},
            {mn.x, mn.y, mx.z},
        };

        glBufferData(
            GL_ARRAY_BUFFER, sizeof(float3) * vx_countof(attribs), &attribs[0], GL_STATIC_DRAW);
    }

    {
        // the aVertex attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, (GLboolean)GL_FALSE, sizeof(float3), 0);
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

void voxel_app_update(const app& app, voxel_app& voxel_app)
{
    if (mouse_button_pressed(button::left))
    {
        auto delta = mouse_delta();
        on_camera_orbit(voxel_app.camera, float(delta.x), float(delta.y), app.time.delta);
    }

    if (scroll_wheel_moved())
    {
        on_camera_dolly(voxel_app.camera, float(-scroll_delta()), app.time.delta);
    }
}

void render_cube(const voxel_app& vox_app, const float3& color, const float4x4& model_matrix)
{
    glUniform3f(2, color.r, color.g, color.b);

    glUniformMatrix4fv(1, 1, GL_FALSE, (const GLfloat*)&model_matrix);

    glBindVertexArray(vox_app.cube_array);
    glDrawArrays(GL_LINES, 0, 24);
    glBindVertexArray(0);
}

void voxel_app_render(const app& app, const voxel_app& vox_app)
{
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);

    glUseProgram(vox_app.shader);

    int w, h;
    SDL_GetWindowSize(app.platform.window, &w, &h);

    float4x4 camera_mat =
        glm::perspective(
            vox_app.camera.fovy, float(w) / h, vox_app.camera.near, vox_app.camera.far) *
        glm::lookAt(eye(vox_app.camera), vox_app.camera.focal_point, up(vox_app.camera));

    glUniformMatrix4fv(0, 1, GL_FALSE, (const GLfloat*)&camera_mat);

    float3 scene_extents = extents(vox_app.scene_bounds);
    float3 voxel_extents = scene_extents / (float)VX_GRID_SIZE;

    for (int z = 0; z < VX_GRID_SIZE; z++)
        for (int y = 0; y < VX_GRID_SIZE; y++)
            for (int x = 0; x < VX_GRID_SIZE; x++)
            {
                int i = x + y * VX_GRID_SIZE + z * VX_GRID_SIZE * VX_GRID_SIZE;
                if (vox_app.voxel_grid[i])
                {
                    float3 voxel_coords{x, y, z};
                    bounds3f voxel_bounds;
                    voxel_bounds.min = vox_app.scene_bounds.min + voxel_coords * voxel_extents;
                    voxel_bounds.max = voxel_bounds.min + voxel_extents;

                    float4x4 model = glm::translate(float4x4{1.f}, center(voxel_bounds)) *
                                     glm::scale(float4x4{1.f}, 0.5f * voxel_extents);

                    float3 voxel_color = voxel_coords / (float)VX_GRID_SIZE;
                    render_cube(vox_app, voxel_color, model);
                }
            }

    ray ray = generate_camera_ray(app, vox_app.camera);
    float closest_t = INFINITY;

    for (int z = 0; z < VX_GRID_SIZE; z++)
        for (int y = 0; y < VX_GRID_SIZE; y++)
            for (int x = 0; x < VX_GRID_SIZE; x++)
            {
                int i = x + y * VX_GRID_SIZE + z * VX_GRID_SIZE * VX_GRID_SIZE;
                if (vox_app.voxel_grid[i])
                {
                    float3 voxel_coords{x, y, z};
                    bounds3f voxel_bounds;
                    voxel_bounds.min = vox_app.scene_bounds.min + voxel_coords * voxel_extents;
                    voxel_bounds.max = voxel_bounds.min + voxel_extents;

                    if (ray_intersects_aabb(ray, voxel_bounds) && ray.t < closest_t)
                        closest_t = ray.t;
                }
            }

    if (closest_t < INFINITY)
    {
        float4x4 model_matrix =
            glm::translate(float4x4{1.f}, ray.origin + ray.direction * closest_t) *
            glm::scale(float4x4{1.f}, float3{0.025f});
        render_cube(vox_app, vox_app.selection_color, model_matrix);
    }

    render_cube(vox_app, vox_app.cube_color, float4x4{});

    glUseProgram(0);
}

void voxel_app_shutdown(voxel_app* vox_app)
{
    glDeleteProgram(vox_app->shader);
    glDeleteBuffers(1, &vox_app->cube_buffer);
    glDeleteVertexArrays(1, &vox_app->cube_array);
}
}
}

int main(int /*argc*/, char** /*argv*/)
{
    //
    // init
    //

    vx::app app;
    vx::voxel_app voxel_app;

    vx::platform_init(&app.platform, app.window.title, app.window.size);

    if (!vx::imgui_init(&app.platform))
        vx::fatal("ImGui initialization failed");

    if (!vx::voxel_app_initialize(voxel_app))
        vx::fatal("Voxel app initialization failed");

    //
    // main loop
    //

    while (app.running)
    {
        // timing

        {
            vx::u64 prev, delta;
            prev = app.time.clocks;
            app.time.clocks = SDL_GetPerformanceCounter();
            delta = app.time.clocks - prev;
            app.time.delta = (float)(delta / (double)SDL_GetPerformanceFrequency());
            app.time.total += app.time.delta;
        }

        // inputs & events

        {
            vx::mouse_update();

            SDL_Event ev;
            while (SDL_PollEvent(&ev))
            {
                switch (ev.type)
                {
                    case SDL_QUIT:
                        app.running = false;
                }

                vx::mouse_handle_event(ev);
                vx::imgui_process_event(&ev);
            }
        }

        // app

        {
            vx::voxel_app_update(app, voxel_app);
        }

        // gui

        {
            vx::imgui_new_frame(app.platform.window);
            ImGui::Begin("Hello, ImGui");
            static float greatness;
            ImGui::SliderFloat("Greatness", &greatness, 0.0f, 1.0f);
            if (ImGui::Button("Yay!"))
                printf("%f\n", greatness);
            ImGui::End();
        }

        // rendering

        vx::platform_frame_begin(&app.platform);

        {
            vx::gpu_device* gpu = app.platform.gpu;
            vx::gpu_channel* channel = vx::gpu_channel_open(gpu);
            vx::gpu_clear_cmd_args clear_args = {app.render.bg_color, 0.0f, 0};
            vx::gpu_channel_clear_cmd(channel, &clear_args);
            vx::imgui_render(&app.platform, channel);
            vx::voxel_app_render(app, voxel_app);
            vx::gpu_channel_close(gpu, channel);
        }

        vx::platform_frame_end(&app.platform);
    }

    //
    // teardown
    //

    vx::voxel_app_shutdown(&voxel_app);
    vx::imgui_shutdown();
    vx::platform_quit(&app.platform);

    return 0;
}
