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

enum voxel_flag
{
    voxel_flag_solid = 1 << 0,
};

struct voxel_leaf
{
    float3 color;
    u32 flags;
};

struct voxel_app
{
    struct mesh
    {
        GLuint vbo{0u}, ibo{0u}, vao{0u};
        u32 vertex_count, index_count;
    };

    mesh wire_cube, solid_cube, quad;
    mesh voxel_grid_rulers[axis_plane_count];

    struct
    {
        GLuint tex{0u};
    } color_wheel;
    GLuint line_shader{0u}, solid_shader{0u}, textured_shader{0u};
    float3 cube_color{0.1f, 0.1f, 0.1f};
    float3 grid_color{0.4f, 0.4f, 0.4f};
    float3 selection_color{0.05f, 0.05f, 0.05f};
    float3 erase_color{1.0f, 0.1f, 0.1f};
    float4x4 camera_view{};
    orbit_camera camera{};

    bounds3f scene_bounds{float3{-1.f}, float3{1.f}};
    float3 scene_extents;
    float3 voxel_extents;
    voxel_leaf voxel_grid[VX_GRID_SIZE * VX_GRID_SIZE * VX_GRID_SIZE];

    // TODO(vinht): This is getting ridiculous.
    voxel_intersect_event intersect;
    bool color_wheel_changed;
    float3 color_wheel_hit_pos;
    float3 color_wheel_rgb;
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
        "#version 440 core\n", "#define VX_SHADER 0\n", shader_source,
    };
    char* fs_defs[] = {
        "#version 440 core\n", "#define VX_SHADER 1\n", shader_source,
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

// hsv2rgb from https://stackoverflow.com/a/19873710
float3 hue(float h)
{
    float r = std::abs(h * 6.f - 3.f) - 1.f;
    float g = 2.f - std::abs(h * 6.f - 2.f);
    float b = 2.f - std::abs(h * 6.f - 4.f);
    return glm::clamp(float3{r, g, b}, float3{0.f}, float3{1.f});
}

float3 hsv_to_rgb(float3 hsv) { return float3(((hue(hsv.x) - 1.f) * hsv.y + 1.f) * hsv.z); }

bool voxel_app_init(platform* platform, voxel_app& vox_app)
{
    //
    // voxel grid
    //

    {
        memset(vox_app.voxel_grid, 0, sizeof(vox_app.voxel_grid));
        printf("%d x %d x %d voxel grid\n", VX_GRID_SIZE, VX_GRID_SIZE, VX_GRID_SIZE);
        printf("%d voxels\n", VX_GRID_SIZE * VX_GRID_SIZE * VX_GRID_SIZE);
        printf("sizeof(voxel_leaf) = %zu\n", sizeof(voxel_leaf));
        printf(
            "sizeof(voxel_grid) = %zu (%.2f MB)\n",
            sizeof(vox_app.voxel_grid),
            sizeof(vox_app.voxel_grid) * 1e-6);
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

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float3), 0);

        vox_app.wire_cube.vao = vao;
        vox_app.wire_cube.vbo = vbo;
        vox_app.wire_cube.ibo = ibo;
        vox_app.wire_cube.vertex_count = vx_countof(corners);
        vox_app.wire_cube.index_count = 2 * vx_countof(lines);
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

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
            1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (const void*)sizeof(float3));

        vox_app.solid_cube.vao = vao;
        vox_app.solid_cube.vbo = vbo;
        vox_app.solid_cube.ibo = ibo;
        vox_app.solid_cube.vertex_count = vx_countof(verts);
        vox_app.solid_cube.index_count = 3 * vx_countof(tris);
    }

    //
    // voxel grid lines
    //

    for (int axis_plane = 0; axis_plane < axis_plane_count; axis_plane++)
    {
        voxel_app::mesh* mesh = &vox_app.voxel_grid_rulers[axis_plane];

        // Two sets (horizontal & vertical) of line caps (begin & end). Number
        // of grid lines is grid size + 1.
        u32 vertex_count = 2 * 2 * (VX_GRID_SIZE + 1);
        mesh->vertex_count = mesh->index_count = vertex_count;
        usize vertex_size = vertex_count * sizeof(float3);
        usize index_size = vertex_count * sizeof(int2);
        float3* vertices = (float3*)malloc(vertex_size);
        int2* indices = (int2*)malloc(index_size);
        float3* vptr = vertices;
        int2* iptr = indices;

        for (int i = 0; i < 2 * (VX_GRID_SIZE + 1); i++)
        {
            int line_index = i >> 1;
            float step = -1.0f + line_index * vox_app.voxel_extents[axis_plane];
            float3 point_a, point_b;
            point_a[axis_plane] = -1.0f, point_b[axis_plane] = +1.0f;
            point_a[(axis_plane + 1) % 3] = point_b[(axis_plane + 1) % 3] = step;
            point_a[(axis_plane + 2) % 3] = point_b[(axis_plane + 2) % 3] = 0.0f;
            if (i & 1)
            {
                std::swap(point_a[axis_plane], point_a[(axis_plane + 1) % 3]);
                std::swap(point_b[axis_plane], point_b[(axis_plane + 1) % 3]);
            }
            *vptr++ = point_a, *vptr++ = point_b;
            *iptr++ = int2(2 * i, 2 * i + 1);
        }

        glGenVertexArrays(1, &mesh->vao);
        glBindVertexArray(mesh->vao);

        glGenBuffers(1, &mesh->vbo);
        glGenBuffers(1, &mesh->ibo);

        glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
        glBufferData(GL_ARRAY_BUFFER, vertex_size, vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_size, indices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float3), 0);

        free(vertices);
        free(indices);
    }

    //
    // quad
    //

    {
        GLuint vao, vbo, ibo;

        struct vertex
        {
            float3 position;
            float2 uv;
        };
        vertex verts[] = {
            {{-1.f, -1.f, 0.f}, {0.f, 0.f}},
            {{+1.f, -1.f, 0.f}, {1.f, 0.f}},
            {{+1.f, +1.f, 0.f}, {1.f, 1.f}},
            {{-1.f, +1.f, 0.f}, {0.f, 1.f}},
        };
        int3 tris[] = {{0, 1, 2}, {2, 3, 0}};

        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ibo);
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(tris), tris, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
            1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (const void*)sizeof(float3));

        vox_app.quad.vao = vao;
        vox_app.quad.vbo = vbo;
        vox_app.quad.ibo = ibo;
        vox_app.quad.vertex_count = vx_countof(verts);
        vox_app.quad.index_count = 3 * vx_countof(tris);
    }

    //
    // color wheel
    //

    {
        // Baking HSV color wheel into a texture.

        i32 w, h;
        u8* pixels;

        w = h = 512;
        pixels = (u8*)malloc(4 * w * h);

        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                int i = x + y * w;
                float2 p = remap_range(         // remap the
                    float2{x + 0.5f, y + 0.5f}, // pixel center
                    float2{0.f, 0.f},           // from
                    float2{w, h},               // image space
                    float2{-1.f, -1.f},         // to
                    float2{1.f, 1.f});          // [-1,1]
                p.y *= -1.f; // NOTE(vinht): In OpenGL, (0,0) starts from lower-left corner!
                float r = glm::length(p);
                if (r < 1.f)
                {
                    float angle =
                        remap_range(glm::atan(p.y, p.x), -float(M_PI), float(M_PI), 0.f, 1.f);
                    float3 rgb = hsv_to_rgb(float3{angle, r, 1.f});
                    for (int c = 0; c < 3; c++)
                        pixels[4 * i + c] = (u8)(255.f * rgb[c]);
                    pixels[4 * i + 3] = 255;
                }
                else
                {
                    for (int c = 0; c < 3; c++)
                        pixels[4 * i + c] = 128;
                    pixels[4 * i + 3] = 255;
                }
            }

        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);
        free(pixels);

        vox_app.color_wheel.tex = tex;
    }

    //
    // shaders
    //

    {
        vox_app.line_shader = compile_gl_shader_from_file("shaders/gl/line.glsl");
        vox_app.solid_shader = compile_gl_shader_from_file("shaders/gl/solid.glsl");
        vox_app.textured_shader = compile_gl_shader_from_file("shaders/gl/textured.glsl");
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
                    if (vox_app.voxel_grid[i].flags)
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

        for (int axis = 0; axis < 3; axis++)
        {
            // TODO(vinht): Remove hard coded planes.

            // NOTE(vinht): Having a tiny extent in the third dimension
            // improves numerical precision in the ray-test. Remapping from
            // intersection point to voxel grid can be done more robustly (no
            // need to handle negative zeroes).
            const float tiny_extent = 1.0f / (1 << 16);
            float t;
            float3 mn, mx;
            mn[axis] = mn[(axis + 1) % 3] = -1.0f;
            mx[axis] = mx[(axis + 1) % 3] = +1.0f;
            mn[(axis + 2) % 3] = -tiny_extent;
            mx[(axis + 2) % 3] = +tiny_extent;

            if (ray_intersects_aabb(ray, bounds3f{mn, mx}, &t) && t < vox_app.intersect.t)
            {
                vox_app.intersect.t = t;
                vox_app.intersect.position = ray.origin + ray.direction * t;
                vox_app.intersect.voxel_coords = (int3)glm::floor(remap_range(
                    vox_app.intersect.position,
                    vox_app.scene_bounds.min,
                    vox_app.scene_bounds.max,
                    float3{0.f},
                    float3{VX_GRID_SIZE}));
                vox_app.intersect.normal = float3{0.f};
            }
        }

        // color wheel

        {
            float3 mn{-1.f, -1.f, -1.f}, mx{1.f, -1.f, 1.f};
            float3 off{2.5f, 0.f, 0.f};
            bounds3f bounds{mn + off, mx + off};
            vox_app.color_wheel_changed = false;
            float t;
            if (ray_intersects_aabb(ray, bounds, &t) && t < vox_app.intersect.t &&
                mouse_button_down(button::left))
            {
                float3 p0 = ray.origin + ray.direction * t;
                float3 p = remap_range(p0, bounds.min, bounds.max, float3{-1.f}, float3{1.f});
                float r = glm::length(float2{p.x, p.z});
                if (r < 1.0f)
                {
                    float angle =
                        remap_range(glm::atan(p.z, p.x), -float(M_PI), float(M_PI), 0.f, 1.f);
                    vox_app.color_wheel_changed = true;
                    vox_app.color_wheel_rgb = hsv_to_rgb(float3{angle, r, 1.f});
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
            const u8* kb = SDL_GetKeyboardState(0);

            if (kb[SDL_SCANCODE_LSHIFT])
            {
                int3 p = vox_app.intersect.voxel_coords;
                if (glm::all(glm::greaterThanEqual(p, int3{0})) &&
                    glm::all(glm::lessThan(p, int3{VX_GRID_SIZE})))
                {
                    i32 i = p.x + p.y * VX_GRID_SIZE + p.z * VX_GRID_SIZE * VX_GRID_SIZE;
                    assert(vox_app.voxel_grid[i].flags & voxel_flag_solid);
                    vox_app.voxel_grid[i].flags = 0;
                }
            }
            else
            {
                int3 p = vox_app.intersect.voxel_coords + (int3)vox_app.intersect.normal;

                if (glm::all(glm::greaterThanEqual(p, int3{0})) &&
                    glm::all(glm::lessThan(p, int3{VX_GRID_SIZE})))
                {
                    fprintf(stdout, "Inserted voxel at %d %d %d\n", p.x, p.y, p.z);
                    i32 i = p.x + p.y * VX_GRID_SIZE + p.z * VX_GRID_SIZE * VX_GRID_SIZE;
                    vox_app.voxel_grid[i].flags |= voxel_flag_solid;
                    vox_app.voxel_grid[i].color = vox_app.color_wheel_rgb;
                }
                else
                    fprintf(
                        stdout,
                        "New voxel at %d %d %d would go outside the boundary! \n",
                        p.x,
                        p.y,
                        p.z);
            }
        }
    }
}

void render_wire_cube(const voxel_app& vox_app, const float3& color, const float4x4& model_matrix)
{
    glUseProgram(vox_app.line_shader);
    glUniform3f(2, color.r, color.g, color.b);
    glUniformMatrix4fv(1, 1, GL_FALSE, (const GLfloat*)&model_matrix);

    glBindVertexArray(vox_app.wire_cube.vao);
    glDrawElements(GL_LINES, vox_app.wire_cube.index_count, GL_UNSIGNED_INT, 0);
}

void render_solid_cube(const voxel_app& vox_app, const float3& color, const float4x4& model_matrix)
{
    glUseProgram(vox_app.solid_shader);
    glUniform3f(2, color.r, color.g, color.b);
    glUniformMatrix4fv(1, 1, GL_FALSE, (const GLfloat*)&model_matrix);

    glBindVertexArray(vox_app.solid_cube.vao);
    glDrawElements(GL_TRIANGLES, vox_app.solid_cube.index_count, GL_UNSIGNED_INT, 0);
}

void render_voxel_grid_lines(
    const voxel_app& vox_app,
    const float3& color,
    const float4x4& model_matrix)
{
    glUseProgram(vox_app.line_shader);
    glUniform3f(2, color.r, color.g, color.b);
    glUniformMatrix4fv(1, 1, GL_FALSE, (const GLfloat*)&model_matrix);

    for (int i = 0; i < vx_countof(vox_app.voxel_grid_rulers); i++)
    {
        const voxel_app::mesh* mesh = &vox_app.voxel_grid_rulers[i];
        glBindVertexArray(mesh->vao);
        glDrawElements(GL_LINES, mesh->index_count, GL_UNSIGNED_INT, 0);
    }
}

void render_textured_quad(const voxel_app& vox_app, GLuint texture, const float4x4& model_matrix)
{
    glUseProgram(vox_app.textured_shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(2, 0);
    glUniformMatrix4fv(1, 1, GL_FALSE, (const GLfloat*)&model_matrix);

    glBindVertexArray(vox_app.quad.vao);
    glDrawElements(GL_TRIANGLES, vox_app.quad.index_count, GL_UNSIGNED_INT, 0);
}

void voxel_app_render(const app&, const voxel_app& vox_app)
{
    //
    // fixed-function state
    //

    {
        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        glDisable(GL_SCISSOR_TEST);

        glClearColor(1, 1, 1, 1);
        glClearDepth(1);
        glClearStencil(0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glFrontFace(GL_CCW);
        glDepthFunc(GL_LEQUAL);
    }

    //
    // common shader constants
    //

    {
        GLuint shaders[] = {vox_app.line_shader, vox_app.solid_shader, vox_app.textured_shader};
        for (int i = 0; i < vx_countof(shaders); i++)
        {
            glUseProgram(shaders[i]);
            glUniformMatrix4fv(0, 1, GL_FALSE, (const GLfloat*)&vox_app.camera.transform);
        }
    }

    //
    // voxels
    //

    for (int z = 0; z < VX_GRID_SIZE; z++)
        for (int y = 0; y < VX_GRID_SIZE; y++)
            for (int x = 0; x < VX_GRID_SIZE; x++)
            {
                int i = x + y * VX_GRID_SIZE + z * VX_GRID_SIZE * VX_GRID_SIZE;
                if (vox_app.voxel_grid[i].flags & voxel_flag_solid)
                {
                    float3 voxel_coords{x, y, z};
                    bounds3f voxel_bounds;
                    voxel_bounds.min =
                        vox_app.scene_bounds.min + voxel_coords * vox_app.voxel_extents;
                    voxel_bounds.max = voxel_bounds.min + vox_app.voxel_extents;

                    float4x4 model = glm::translate(float4x4{1.f}, center(voxel_bounds)) *
                                     glm::scale(float4x4{1.f}, 0.5f * vox_app.voxel_extents);

                    render_solid_cube(vox_app, vox_app.voxel_grid[i].color, model);
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

        const u8* kb = SDL_GetKeyboardState(0);

        if (kb[SDL_SCANCODE_LSHIFT])
        {
            render_wire_cube(
                vox_app,
                vox_app.erase_color,
                glm::translate(
                    float4x4{1.f},
                    center(reconstruct_voxel_bounds(
                        vox_app.intersect.voxel_coords, vox_app.scene_bounds, VX_GRID_SIZE))) *
                    glm::scale(float4x4{1.f}, 0.5f * vox_app.voxel_extents));
        }
        else
        {
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
    }

    //
    // color wheel
    //

    render_textured_quad(
        vox_app,
        vox_app.color_wheel.tex,
        glm::translate(float4x4{1.f}, float3{2.5f, -1.f, 0.f}) *
            glm::rotate(float4x4{1.f}, -(float)M_PI / 2.0f, float3{1.f, 0.f, 0.f}));

    //
    // grid lines
    //

    {
        glDepthMask(GL_FALSE);
        render_voxel_grid_lines(vox_app, vox_app.grid_color, float4x4{});
        render_wire_cube(vox_app, vox_app.cube_color, float4x4{});
        glDepthMask(GL_TRUE);
    }
}

void voxel_app_shutdown(voxel_app* vox_app)
{
    // TODO(vinht): Free GPU resources.
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

    if (!vx::voxel_app_init(&app.platform, voxel_app))
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
            vx::voxel_app_render(app, voxel_app);
            vx::imgui_render(&app.platform, channel);
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
