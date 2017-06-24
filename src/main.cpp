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

template<typename VecType>
VecType remap_range(VecType value, VecType a_min, VecType a_max, VecType b_min, VecType b_max)
{
    return b_min + (value - a_min) * (b_max - b_min) / (a_max - a_min);
}

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

    float4x4 transform;
};

#define VX_GRID_SIZE 16

struct voxel_intersect_event
{
    float t = INFINITY;
    float3 position, normal;
    int3 voxel_coords;
};

struct voxel_app
{
    struct
    {
        GLuint vbo{0u}, ibo{0u}, vao{0u};
    } wire_cube, solid_cube;
    struct
    {
        GLuint vbo{0u}, vao{0u};
        u32 vertex_count;
    } voxel_grid_lines;
    GLuint line_shader{0u}, solid_shader{0u};
    float3 cube_color{0.1f, 0.1f, 0.1f};
    float3 grid_color{0.4f, 0.4f, 0.4f};
    float3 selection_color{0.05f, 0.05f, 0.05f};
    float4x4 camera_view{};
    orbit_camera camera{};

    bounds3f scene_bounds{float3{-1.f}, float3{1.f}};
    float3 scene_extents;
    float3 voxel_extents;
    u8 voxel_grid[VX_GRID_SIZE * VX_GRID_SIZE * VX_GRID_SIZE];

    voxel_intersect_event intersect;
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

bool voxel_app_init(voxel_app& vox_app)
{
    //
    // voxel grid
    //

    {
        memset(vox_app.voxel_grid, 0, sizeof(vox_app.voxel_grid));
        vox_app.scene_extents = extents(vox_app.scene_bounds);
        vox_app.voxel_extents = vox_app.scene_extents / (float)VX_GRID_SIZE;
    }

    //
    // wire cube
    //

    {
        GLuint vao, vbo, ibo;
        float3 mn{-1.0f}, mx{+1.0f};

        // clang-format off
        float3 corners[] = {
            {mn.x, mn.y, mn.z}, {mn.x, mn.y, mx.z}, {mx.x, mn.y, mx.z}, {mx.x, mn.y, mn.z},
            {mn.x, mx.y, mn.z}, {mn.x, mx.y, mx.z}, {mx.x, mx.y, mx.z}, {mx.x, mx.y, mn.z},
        };
        uint2 lines[] = {
            {0, 1}, {1, 2}, {2, 3}, {3, 0},
            {4, 5}, {5, 6}, {6, 7}, {7, 4},
            {0, 4}, {1, 5}, {2, 6}, {3, 7},
        };
        // clang-format on

        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ibo);
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(corners), corners, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(lines), lines, GL_STATIC_DRAW);

        glEnableVertexArrayAttrib(vao, 0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float3), 0);

        vox_app.wire_cube.vao = vao;
        vox_app.wire_cube.vbo = vbo;
        vox_app.wire_cube.ibo = ibo;
    }

    //
    // solid cube
    //

    {
        GLuint vao, vbo, ibo;
        float3 mn{-1.0f}, mx{+1.0f};

        struct vertex
        {
            float3 position, normal;
        };

        // clang-format off
        vertex verts[] =
        {
            // -x
            {{mn.x, mn.y, mn.z}, {-1.f, 0.f, 0.f}}, // 0
            {{mn.x, mn.y, mx.z}, {-1.f, 0.f, 0.f}}, // 1
            {{mn.x, mx.y, mx.z}, {-1.f, 0.f, 0.f}}, // 2
            {{mn.x, mx.y, mn.z}, {-1.f, 0.f, 0.f}}, // 3
            // +x
            {{mx.x, mx.y, mn.z}, {1.f, 0.f, 0.f}}, // 4
            {{mx.x, mx.y, mx.z}, {1.f, 0.f, 0.f}}, // 5
            {{mx.x, mn.y, mx.z}, {1.f, 0.f, 0.f}}, // 6
            {{mx.x, mn.y, mn.z}, {1.f, 0.f, 0.f}}, // 7
            // -y
            {{mx.x, mn.y, mn.z}, {0.f, -1.f, 0.f}}, // 8
            {{mx.x, mn.y, mx.z}, {0.f, -1.f, 0.f}}, // 9
            {{mn.x, mn.y, mx.z}, {0.f, -1.f, 0.f}}, // 10
            {{mn.x, mn.y, mn.z}, {0.f, -1.f, 0.f}}, // 11
            // +y
            {{mn.x, mx.y, mn.z}, {0.f, 1.f, 0.f}}, // 12
            {{mn.x, mx.y, mx.z}, {0.f, 1.f, 0.f}}, // 13
            {{mx.x, mx.y, mx.z}, {0.f, 1.f, 0.f}}, // 14
            {{mx.x, mx.y, mn.z}, {0.f, 1.f, 0.f}}, // 15
            // -z
            {{mn.x, mx.y, mn.z}, {0.f, 0.f, -1.f}}, // 16
            {{mx.x, mx.y, mn.z}, {0.f, 0.f, -1.f}}, // 17
            {{mx.x, mn.y, mn.z}, {0.f, 0.f, -1.f}}, // 18
            {{mn.x, mn.y, mn.z}, {0.f, 0.f, -1.f}}, // 19
            // +z
            {{mn.x, mn.y, mx.z}, {0.f, 0.f, 1.f}}, // 20
            {{mx.x, mn.y, mx.z}, {0.f, 0.f, 1.f}}, // 21
            {{mx.x, mx.y, mx.z}, {0.f, 0.f, 1.f}}, // 22
            {{mn.x, mx.y, mx.z}, {0.f, 0.f, 1.f}}, // 23
        };
        int3 tris[] =
        {
            {0,1,2}, {2,3,0}, // -x
            {4,5,6}, {6,7,4}, // +x
            {8,9,10}, {10,11,8}, // -y
            {12,13,14}, {14,15,12}, // +y
            {16,17,18}, {18,19,16}, // -z
            {20,21,22}, {22,23,20}, // +z
        };
        // clang-format on

        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ibo);
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(tris), tris, GL_STATIC_DRAW);

        glEnableVertexArrayAttrib(vao, 0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
        glEnableVertexArrayAttrib(vao, 1);
        glVertexAttribPointer(
            1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (const void*)sizeof(float3));

        vox_app.solid_cube.vao = vao;
        vox_app.solid_cube.vbo = vbo;
        vox_app.solid_cube.ibo = ibo;
    }

    //
    // voxel grid lines
    //

    {
        GLuint vbo, vao;
        float3 mn{-1.0f}, mx{+1.0f};
        float3 scene_extents = extents(vox_app.scene_bounds);
        float3 voxel_extents = scene_extents / (float)VX_GRID_SIZE;

        // The grid is constructed on xz-plane with the grid pointing up
        // towards +y. Two sets (horizontal & vertical) of line caps (begin &
        // end). Number of grid lines is grid size + 1.
        u32 vertex_count = 2 * 2 * (VX_GRID_SIZE + 1);
        usize vertex_size = vertex_count * sizeof(float3);
        float3* vertices = (float3*)malloc(vertex_size);
        float3* vptr = vertices;
        for (int i = 0; i < VX_GRID_SIZE + 1; i++)
        {
            *vptr++ = float3{mn.x + i * voxel_extents.x, 0.0f, mn.z};
            *vptr++ = float3{mn.x + i * voxel_extents.x, 0.0f, mx.z};
        }
        for (int i = 0; i < VX_GRID_SIZE + 1; i++)
        {
            *vptr++ = float3{mn.x, 0.0f, mn.z + i * voxel_extents.z};
            *vptr++ = float3{mx.x, 0.0f, mn.z + i * voxel_extents.z};
        }

        glGenBuffers(1, &vbo);
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertex_size, vertices, GL_STATIC_DRAW);

        glEnableVertexArrayAttrib(vao, 0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float3), 0);

        free(vertices);

        vox_app.voxel_grid_lines.vbo = vbo;
        vox_app.voxel_grid_lines.vao = vao;
        vox_app.voxel_grid_lines.vertex_count = vertex_count;
    }

    //
    // shaders
    //

    {
        vox_app.line_shader = compile_gl_shader_from_file("src/shaders/gl/line.glsl");
        vox_app.solid_shader = compile_gl_shader_from_file("src/shaders/gl/solid.glsl");
    }

    return true;
}

float3 reconstruct_voxel_normal(const float3& voxel_center, const float3& point_on_voxel)
{
    // Reconstruct hit normal from voxel center & hit point by finding
    // highest magnitude coefficient from their difference, zeroing other
    // coefficients and extracting the sign of the highest coefficient.

    float3 normal = glm::normalize(point_on_voxel - voxel_center);
    int max_coeff_index = 0;
    float max_coeff = 0.0f;

    for (int i = 0; i < 3; i++)
        if (std::abs(normal[i]) > max_coeff)
        {
            max_coeff = std::abs(normal[i]);
            max_coeff_index = i;
        }

    for (int i = 0; i < 3; i++)
        normal[i] = (i == max_coeff_index) ? glm::sign(normal[i]) : 0.0f;

    return normal;
}

bounds3f reconstruct_voxel_bounds(
    const int3& voxel_coords,
    const bounds3f& voxel_grid_bounds,
    i32 voxel_grid_resolution)
{
    float3 scene_extents = extents(voxel_grid_bounds);
    float3 voxel_extents = scene_extents / (float)voxel_grid_resolution;

    bounds3f voxel_bounds;
    voxel_bounds.min = voxel_grid_bounds.min + float3{voxel_coords} * voxel_extents;
    voxel_bounds.max = voxel_bounds.min + voxel_extents;
    return voxel_bounds;
}

void voxel_app_update(const app& app, voxel_app& vox_app)
{
    //
    // camera controls
    //

    {
        if (mouse_button_pressed(button::right))
        {
            auto delta = mouse_delta();
            on_camera_orbit(vox_app.camera, float(delta.x), float(delta.y), app.time.delta);
        }

        if (scroll_wheel_moved())
        {
            on_camera_dolly(vox_app.camera, float(-scroll_delta()), app.time.delta);
        }

        int w, h;
        SDL_GetWindowSize(app.platform.window, &w, &h);

        vox_app.camera.transform =
            glm::perspective(
                vox_app.camera.fovy, float(w) / h, vox_app.camera.near, vox_app.camera.far) *
            glm::lookAt(eye(vox_app.camera), vox_app.camera.focal_point, up(vox_app.camera));
    }

    //
    // intersect scene
    //

    {
        vox_app.intersect = voxel_intersect_event{};

        ray ray = generate_camera_ray(app, vox_app.camera);

        // voxel grid

        for (int z = 0; z < VX_GRID_SIZE; z++)
            for (int y = 0; y < VX_GRID_SIZE; y++)
                for (int x = 0; x < VX_GRID_SIZE; x++)
                {
                    int i = x + y * VX_GRID_SIZE + z * VX_GRID_SIZE * VX_GRID_SIZE;
                    if (vox_app.voxel_grid[i])
                    {
                        float3 voxel_coords{x, y, z};
                        bounds3f voxel_bounds;
                        voxel_bounds.min =
                            vox_app.scene_bounds.min + voxel_coords * vox_app.voxel_extents;
                        voxel_bounds.max = voxel_bounds.min + vox_app.voxel_extents;

                        float t;
                        if (ray_intersects_aabb(ray, voxel_bounds, &t) && t < vox_app.intersect.t)
                        {
                            vox_app.intersect.t = t;
                            vox_app.intersect.voxel_coords = voxel_coords;
                            vox_app.intersect.position = ray.origin + ray.direction * t;
                            vox_app.intersect.normal = reconstruct_voxel_normal(
                                center(voxel_bounds), vox_app.intersect.position);
                        }
                    }
                }

        // voxel rulers

        if (vox_app.intersect.t == INFINITY)
        {
            for (int i = 0; i < 3; i++)
            {
                // TODO(vinht): Remove hard coded planes.

                float t;
                float3 mn{-1.f}, mx{+1.f};
                mx[i] = -1.f;

                if (ray_intersects_aabb(ray, bounds3f{mn, mx}, &t) && t < vox_app.intersect.t)
                {
                    vox_app.intersect.t = t;
                    vox_app.intersect.position = ray.origin + ray.direction * t;

                    int3 v = (int3)glm::floor(remap_range(
                        vox_app.intersect.position,
                        vox_app.scene_bounds.min,
                        vox_app.scene_bounds.max,
                        float3{0.f},
                        float3{VX_GRID_SIZE}));

                    v[i] = -1;

                    vox_app.intersect.voxel_coords = v;
                    vox_app.intersect.normal = float3{0.f};
                    vox_app.intersect.normal[i] = 1.f;
                }
            }
        }
    }

    //
    // voxel editing
    //

    if (vox_app.intersect.t < INFINITY)
    {
        if (mouse_button_down(button::left))
        {
            int3 proposed_voxel = vox_app.intersect.voxel_coords + (int3)vox_app.intersect.normal;
            int3 new_voxel = glm::clamp(
                proposed_voxel,
                int3(0, 0, 0),
                int3(VX_GRID_SIZE - 1, VX_GRID_SIZE - 1, VX_GRID_SIZE - 1));

            if (new_voxel != vox_app.intersect.voxel_coords)
            {
                fprintf(
                    stdout, "Inserted voxel at %d %d %d\n", new_voxel.x, new_voxel.y, new_voxel.z);
                vox_app.voxel_grid
                    [new_voxel.x + new_voxel.y * VX_GRID_SIZE +
                     new_voxel.z * VX_GRID_SIZE * VX_GRID_SIZE] = 1;
            }
            else
                fprintf(
                    stdout,
                    "New voxel at %d %d %d would go outside the boundary! \n",
                    proposed_voxel.x,
                    proposed_voxel.y,
                    proposed_voxel.z);
        }
    }
}

void render_wire_cube(const voxel_app& vox_app, const float3& color, const float4x4& model_matrix)
{
    glUseProgram(vox_app.line_shader);
    glUniform3f(2, color.r, color.g, color.b);
    glUniformMatrix4fv(1, 1, GL_FALSE, (const GLfloat*)&model_matrix);

    glBindVertexArray(vox_app.wire_cube.vao);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
}

void render_solid_cube(const voxel_app& vox_app, const float3& color, const float4x4& model_matrix)
{
    glUseProgram(vox_app.solid_shader);
    glUniform3f(2, color.r, color.g, color.b);
    glUniformMatrix4fv(1, 1, GL_FALSE, (const GLfloat*)&model_matrix);

    glBindVertexArray(vox_app.solid_cube.vao);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}

void render_voxel_grid_lines(
    const voxel_app& vox_app,
    const float3& color,
    const float4x4& model_matrix)
{
    glUseProgram(vox_app.line_shader);
    glUniform3f(2, color.r, color.g, color.b);
    glUniformMatrix4fv(1, 1, GL_FALSE, (const GLfloat*)&model_matrix);

    glBindVertexArray(vox_app.voxel_grid_lines.vao);
    glDrawArrays(GL_LINES, 0, vox_app.voxel_grid_lines.vertex_count);
}

void voxel_app_render(const app&, const voxel_app& vox_app)
{
    //
    // fixed-function state
    //

    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_FRAMEBUFFER_SRGB);

        glFrontFace(GL_CCW);
        glDepthFunc(GL_LEQUAL);
    }

    //
    // common shader constants
    //

    {
        glUseProgram(vox_app.line_shader);
        glUniformMatrix4fv(0, 1, GL_FALSE, (const GLfloat*)&vox_app.camera.transform);
        glUseProgram(vox_app.solid_shader);
        glUniformMatrix4fv(0, 1, GL_FALSE, (const GLfloat*)&vox_app.camera.transform);
    }

    //
    // voxels
    //

    for (int z = 0; z < VX_GRID_SIZE; z++)
        for (int y = 0; y < VX_GRID_SIZE; y++)
            for (int x = 0; x < VX_GRID_SIZE; x++)
            {
                int i = x + y * VX_GRID_SIZE + z * VX_GRID_SIZE * VX_GRID_SIZE;
                if (vox_app.voxel_grid[i])
                {
                    float3 voxel_coords{x, y, z};
                    bounds3f voxel_bounds;
                    voxel_bounds.min =
                        vox_app.scene_bounds.min + voxel_coords * vox_app.voxel_extents;
                    voxel_bounds.max = voxel_bounds.min + vox_app.voxel_extents;

                    float4x4 model = glm::translate(float4x4{1.f}, center(voxel_bounds)) *
                                     glm::scale(float4x4{1.f}, 0.5f * vox_app.voxel_extents);

                    float3 voxel_color = voxel_coords / (float)VX_GRID_SIZE;
                    render_solid_cube(vox_app, voxel_color, model);
                }
            }

    //
    // voxel cursor
    //

    if (vox_app.intersect.t < INFINITY)
    {
        render_solid_cube(
            vox_app,
            vox_app.selection_color,
            glm::translate(float4x4{1.f}, vox_app.intersect.position) *
                glm::scale(float4x4{1.f}, float3{0.005f}));

        render_wire_cube(
            vox_app,
            vox_app.selection_color,
            glm::translate(
                float4x4{1.f},
                center(reconstruct_voxel_bounds(
                    vox_app.intersect.voxel_coords, vox_app.scene_bounds, VX_GRID_SIZE)) +
                    vox_app.voxel_extents * vox_app.intersect.normal) *
                glm::scale(float4x4{1.f}, 0.5f * vox_app.voxel_extents));
    }

    //
    // grid lines
    //

    {
        glDepthMask(GL_FALSE);
        render_voxel_grid_lines(
            vox_app, vox_app.grid_color, glm::translate(float4x4{}, float3{0.0f, -1.0f, 0.0f}));
        render_voxel_grid_lines(
            vox_app,
            vox_app.grid_color,
            glm::translate(float4x4{}, float3{-1.0f, 0.0f, 0.0f}) *
                glm::rotate(float4x4{}, (float)M_PI / 2.0f, float3{0.0f, 0.0f, 1.0f}));
        render_voxel_grid_lines(
            vox_app,
            vox_app.grid_color,
            glm::translate(float4x4{}, float3{0.0f, 0.0f, -1.0f}) *
                glm::rotate(float4x4{}, (float)M_PI / 2.0f, float3{1.0f, 0.0f, 0.0f}));
        render_wire_cube(vox_app, vox_app.cube_color, float4x4{});
        glDepthMask(GL_TRUE);
    }
}

void voxel_app_shutdown(voxel_app* vox_app)
{
    glDeleteProgram(vox_app->line_shader);
    glDeleteProgram(vox_app->solid_shader);
    glDeleteBuffers(1, &vox_app->wire_cube.vbo);
    glDeleteVertexArrays(1, &vox_app->wire_cube.vao);
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

    if (!vx::voxel_app_init(voxel_app))
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
            vx::gpu_clear_cmd_args clear_args = {app.render.bg_color, 1.0f, 0};
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
