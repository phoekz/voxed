#include "common/error.h"
#include "common/geometry.h"
#include "common/intersection.h"
#include "common/mouse.h"
#include "platform/gpu.h"

#include "imgui.h"
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

#define vx_countof(arr) (sizeof(arr) / sizeof(arr[0]))
#define VX_GRID_SIZE 16

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
        int2 size = int2(1600, 1200);
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

const char* vertex_shader =
    "#version 420 core\n"

    "uniform mat4 cameraMatrix;\n"
    "uniform mat4 modelMatrix;\n"

    "layout(location = 0) in vec3 aVertex;\n"

    "void main()\n"
    "{\n"
    "    gl_Position = cameraMatrix * modelMatrix * vec4(aVertex, 1.0);\n"
    "}\n";

const char* fragment_shader =
    "#version 420 core\n"

    "uniform vec3 color;\n"

    "out vec4 fragColor;\n"

    "void main()\n"
    "{\n"
    "    fragColor = vec4(color, 1.0);\n"
    "}\n";

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

struct voxel_app
{
    GLuint cube_buffer{0u};
    GLuint cube_array{0u};
    GLuint shader{0u};
    float3 cube_color{0.1f, 0.1f, 0.1f};
    float4x4 camera_view{};
    orbit_camera camera{};
};

void on_camera_dolly(orbit_camera& camera, float dz, float dt)
{
    const float scale = 2.f;
    // the delta radius is scaled here by the radius itself, so that the scaling acts faster
    // the further away the camera is
    camera.radius -= dt * scale * dz * camera.radius;
    camera.radius = clamp(camera.radius, camera.min_radius, camera.max_radius);
}

void on_camera_orbit(orbit_camera& camera, float dx, float dy, float dt)
{
    const float scale = 4.f;
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

bool initialize_voxel_app(voxel_app& vox_app)
{
    vox_app.shader = glCreateProgram();
    if (vox_app.shader == 0u)
    {
        return false;
    }

    // vertex shader
    {
        GLuint vertex_stage = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex_stage, 1, &vertex_shader, 0);

        glCompileShader(vertex_stage);

        GLint status;
        glGetShaderiv(vertex_stage, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE)
        {
            return false;
        }

        glAttachShader(vox_app.shader, vertex_stage);
        glDeleteShader(vertex_stage);
    }
    // fragment shader
    {
        GLuint fragment_stage = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment_stage, 1, &fragment_shader, 0);

        glCompileShader(fragment_stage);

        GLint status;
        glGetShaderiv(fragment_stage, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE)
        {
            return false;
        }

        glAttachShader(vox_app.shader, fragment_stage);
        glDeleteShader(fragment_stage);
    }
    // link the program
    {
        glLinkProgram(vox_app.shader);
        GLint status;
        glGetProgramiv(vox_app.shader, GL_LINK_STATUS, &status);
        if (status == GL_FALSE)
        {
            return false;
        }
    }

    glGenBuffers(1, &vox_app.cube_buffer);

    glGenVertexArrays(1, &vox_app.cube_array);
    glBindVertexArray(vox_app.cube_array);
    {
        glBindBuffer(GL_ARRAY_BUFFER, vox_app.cube_buffer);

        // the min and max corners of the cube to render
        float3 mn{-1.f, -1.f, -1.f};
        float3 mx{1.f, 1.f, 1.f};

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

void update_voxel_app(const app& app, voxel_app& voxel_app)
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

void render_cube(
    const app& app,
    const voxel_app& vox_app,
    const float3& color,
    const float4x4& model_matrix)
{
    glUniform3f(glGetUniformLocation(vox_app.shader, "color"), color.r, color.g, color.b);

    glUniformMatrix4fv(
        glGetUniformLocation(vox_app.shader, "modelMatrix"),
        1,
        GL_FALSE,
        (const GLfloat*)&model_matrix);

    glBindVertexArray(vox_app.cube_array);
    glDrawArrays(GL_LINES, 0, 24);
    glBindVertexArray(0);
}

void render_voxel_app(const app& app, const voxel_app& vox_app)
{
    glUseProgram(vox_app.shader);

    int w, h;
    SDL_GetWindowSize(app.platform.window, &w, &h);

    float4x4 camera_mat =
        glm::perspective(
            vox_app.camera.fovy, float(w) / h, vox_app.camera.near, vox_app.camera.far) *
        glm::lookAt(eye(vox_app.camera), vox_app.camera.focal_point, up(vox_app.camera));

    glUniformMatrix4fv(
        glGetUniformLocation(vox_app.shader, "cameraMatrix"),
        1,
        GL_FALSE,
        (const GLfloat*)&camera_mat);

    render_cube(app, vox_app, vox_app.cube_color, float4x4{});

    ray ray = generate_camera_ray(app, vox_app.camera);
    if (ray_intersects_aabb(ray, bounds3f{float3{-1.f, -1.f, -1.f}, float3{1.f, 1.f, 1.f}}))
    {
        float4x4 model_matrix = glm::translate(float4x4(1.f), ray.origin + ray.direction * ray.t) *
                                glm::scale(float4x4(1.f), float3(0.1f));
        render_cube(app, vox_app, float3(1.f, 0.2f, 0.2f), model_matrix);
    }

    glUseProgram(0);
}

void shutdown_voxel_app(voxel_app& vox_app)
{
    glDeleteProgram(vox_app.shader);
    glDeleteBuffers(1, &vox_app.cube_buffer);
    glDeleteVertexArrays(1, &vox_app.cube_array);
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

    if (!vx::imgui_init(app.platform.window))
        vx::fatal("ImGui initialization failed");

    if (!vx::initialize_voxel_app(voxel_app))
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

        // gui updates

        {
            vx::imgui_new_frame(app.platform.window);
            vx::update_voxel_app(app, voxel_app);

            ImGui::Begin("Hello, ImGui");
            ImGui::End();
        }

        // rendering

        {
            vx::gpu_device* gpu = app.platform.gpu;
            vx::gpu_channel* channel = vx::gpu_channel_open(gpu);
            vx::gpu_clear_cmd_args clear_args = {app.render.bg_color, 0.0f, 0};
            vx::gpu_channel_clear_cmd(channel, &clear_args);
            vx::gpu_channel_close(gpu, channel);

            vx::render_voxel_app(app, voxel_app);

            ImGui::Render();
        }

        vx::platform_swap_buffers(&app.platform);
    }

    //
    // teardown
    //

    vx::shutdown_voxel_app(voxel_app);
    vx::imgui_shutdown();
    vx::platform_quit(&app.platform);

    return 0;
}
