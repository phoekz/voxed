#include "voxed.h"
#include "common/error.h"
#include "common/intersection.h"
#include "common/math_utils.h"
#include "common/macros.h"
#include "common/mouse.h"
#include "platform/filesystem.h"

#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "integrations/gl/gl.h"

#include <vector>

#undef near
#undef far

#define VX_GRID_SIZE 16

namespace vx
{
namespace
{
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

ray generate_camera_ray(const platform& platform, const orbit_camera& camera)
{
    ray ray;
    ray.origin = eye(camera);

    int w, h;
    SDL_GetWindowSize(platform.window, &w, &h);
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

u32 compile_gl_shader_from_file(const char* file_path)
{
    i32 status;
    char* shader_source = read_whole_file(file_path, nullptr);
    u32 vs_stage = glCreateShader(GL_VERTEX_SHADER);
    u32 fs_stage = glCreateShader(GL_FRAGMENT_SHADER);
    u32 program = glCreateProgram();
    const char* vs_defs[] = {
        "#version 440 core\n", "#define VX_SHADER 0\n", shader_source,
    };
    const char* fs_defs[] = {
        "#version 440 core\n", "#define VX_SHADER 1\n", shader_source,
    };

    glShaderSource(vs_stage, vx_countof(vs_defs), (char**)vs_defs, 0);
    glShaderSource(fs_stage, vx_countof(fs_defs), (char**)fs_defs, 0);

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

    std::free(shader_source);
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
}

struct voxed_state
{
    struct mesh
    {
        u32 vbo{0u}, ibo{0u}, vao{0u};
        u32 vertex_count, index_count;
    };

    mesh wire_cube, solid_cube, quad;
    mesh voxel_grid_rulers[axis_plane_count];

    struct
    {
        u32 tex{0u};
    } color_wheel;
    u32 line_shader{0u}, solid_shader{0u}, textured_shader{0u};
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

    voxed_mode mode;
};

voxed_state* voxed_create()
{
    // TODO(johann): figure out a better way to set default values than calling the constructor
    voxed_state* state = new voxed_state();

    //
    // voxel grid
    //

    {
        memset(state->voxel_grid, 0, sizeof(state->voxel_grid));
        printf("%d x %d x %d voxel grid\n", VX_GRID_SIZE, VX_GRID_SIZE, VX_GRID_SIZE);
        printf("%d voxels\n", VX_GRID_SIZE * VX_GRID_SIZE * VX_GRID_SIZE);
        printf("sizeof(voxel_leaf) = %zu\n", sizeof(voxel_leaf));
        printf(
            "sizeof(voxel_grid) = %zu (%.2f MB)\n",
            sizeof(state->voxel_grid),
            sizeof(state->voxel_grid) * 1e-6);
        state->scene_extents = extents(state->scene_bounds);
        state->voxel_extents = state->scene_extents / (float)VX_GRID_SIZE;
    }

    //
    // wire cube
    //

    {
        u32 vao, vbo, ibo;
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

        state->wire_cube.vao = vao;
        state->wire_cube.vbo = vbo;
        state->wire_cube.ibo = ibo;
        state->wire_cube.vertex_count = vx_countof(corners);
        state->wire_cube.index_count = 2 * vx_countof(lines);
    }

    //
    // solid cube
    //

    {
        u32 vao, vbo, ibo;
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
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)sizeof(float3));

        state->solid_cube.vao = vao;
        state->solid_cube.vbo = vbo;
        state->solid_cube.ibo = ibo;
        state->solid_cube.vertex_count = vx_countof(verts);
        state->solid_cube.index_count = 3 * vx_countof(tris);
    }

    //
    // voxel grid lines
    //

    struct line
    {
        float3 start{0.f};
        float3 end{0.f};
    };

    // assume the extents are the same in all directions
    const float voxel_extent = state->voxel_extents.x;
    const int line_count = (VX_GRID_SIZE / 2) + 1;
    const float line_length = (VX_GRID_SIZE / 2) * voxel_extent;

    std::vector<line> grid_lines;
    grid_lines.reserve(axis_plane_count * 2 * line_count);

    for (int i = 0; i < axis_plane_count; i++)
    {
        for (int j = 0; j < line_count; j++)
        {
            line l1, l2;
            l1.start = float3(0.f);
            l1.end = float3(0.f);
            l1.start[(i + 1) % 3] = voxel_extent * j;
            l1.end[(i + 1) % 3] = voxel_extent * j;
            l1.end[i] = line_length;

            grid_lines.push_back(l1);

            l2.start[i] = voxel_extent * j;
            l2.end[i] = voxel_extent * j;
            l2.end[(i + 1) % 3] = line_length;

            grid_lines.push_back(l2);
        }

        auto& mesh = state->voxel_grid_rulers[i];
        mesh.index_count = u32(grid_lines.size() * 2);
        glGenVertexArrays(1, &mesh.vao);
        glBindVertexArray(mesh.vao);

        glGenBuffers(1, &mesh.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
        glBufferData(
            GL_ARRAY_BUFFER, sizeof(line) * grid_lines.size(), grid_lines.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float3), 0);

        grid_lines.clear();
    }

    //
    // quad
    //

    {
        u32 vao, vbo, ibo;

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
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)sizeof(float3));

        state->quad.vao = vao;
        state->quad.vbo = vbo;
        state->quad.ibo = ibo;
        state->quad.vertex_count = vx_countof(verts);
        state->quad.index_count = 3 * vx_countof(tris);
    }

    //
    // color wheel
    //

    {
        // Baking HSV color wheel into a texture.

        i32 w, h;
        u8* pixels;

        w = h = 512;
        pixels = (u8*)std::malloc(4 * w * h);

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

        u32 tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);
        std::free(pixels);

        state->color_wheel.tex = tex;
    }

    //
    // shaders
    //

    {
        state->line_shader = compile_gl_shader_from_file("shaders/gl/line.glsl");
        state->solid_shader = compile_gl_shader_from_file("shaders/gl/solid.glsl");
        state->textured_shader = compile_gl_shader_from_file("shaders/gl/textured.glsl");
    }

    return state;
}

void voxed_set_mode(voxed_state* state, voxed_mode mode) { state->mode = mode; }

void voxed_update(voxed_state* state, const platform& platform, float dt)
{
    //
    // camera controls
    //

    {
        if (mouse_button_pressed(button::right))
        {
            auto delta = mouse_delta();
            on_camera_orbit(state->camera, float(delta.x), float(delta.y), dt);
        }

        if (scroll_wheel_moved())
        {
            on_camera_dolly(state->camera, float(-scroll_delta()), dt);
        }

        int w, h;
        SDL_GetWindowSize(platform.window, &w, &h);

        state->camera.transform =
            glm::perspective(
                state->camera.fovy, float(w) / h, state->camera.near, state->camera.far) *
            glm::lookAt(eye(state->camera), state->camera.focal_point, up(state->camera));
    }

    //
    // intersect scene
    //

    {
        state->intersect = voxel_intersect_event{};

        ray ray = generate_camera_ray(platform, state->camera);

        // voxel grid

        for (int z = 0; z < VX_GRID_SIZE; z++)
            for (int y = 0; y < VX_GRID_SIZE; y++)
                for (int x = 0; x < VX_GRID_SIZE; x++)
                {
                    int i = x + y * VX_GRID_SIZE + z * VX_GRID_SIZE * VX_GRID_SIZE;
                    if (state->voxel_grid[i].flags)
                    {
                        float3 voxel_coords{x, y, z};
                        bounds3f voxel_bounds;
                        voxel_bounds.min =
                            state->scene_bounds.min + voxel_coords * state->voxel_extents;
                        voxel_bounds.max = voxel_bounds.min + state->voxel_extents;

                        float t;
                        if (ray_intersects_aabb(ray, voxel_bounds, &t) && t < state->intersect.t)
                        {
                            state->intersect.t = t;
                            state->intersect.voxel_coords = voxel_coords;
                            state->intersect.position = ray.origin + ray.direction * t;
                            state->intersect.normal = reconstruct_voxel_normal(
                                center(voxel_bounds), state->intersect.position);
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

            if (ray_intersects_aabb(ray, bounds3f{mn, mx}, &t) && t < state->intersect.t)
            {
                state->intersect.t = t;
                state->intersect.position = ray.origin + ray.direction * t;
                state->intersect.voxel_coords = (int3)glm::floor(remap_range(
                    state->intersect.position,
                    state->scene_bounds.min,
                    state->scene_bounds.max,
                    float3{0.f},
                    float3{VX_GRID_SIZE}));
                state->intersect.normal = float3{0.f};
            }
        }

        // color wheel

        {
            float3 mn{-1.f, -1.f, -1.f}, mx{1.f, -1.f, 1.f};
            float3 off{2.5f, 0.f, 0.f};
            bounds3f bounds{mn + off, mx + off};
            state->color_wheel_changed = false;
            float t;
            if (ray_intersects_aabb(ray, bounds, &t) && t < state->intersect.t &&
                mouse_button_down(button::left))
            {
                float3 p0 = ray.origin + ray.direction * t;
                float3 p = remap_range(p0, bounds.min, bounds.max, float3{-1.f}, float3{1.f});
                float r = glm::length(float2{p.x, p.z});
                if (r < 1.0f)
                {
                    float angle =
                        remap_range(glm::atan(p.z, p.x), -float(M_PI), float(M_PI), 0.f, 1.f);
                    state->color_wheel_changed = true;
                    state->color_wheel_rgb = hsv_to_rgb(float3{angle, r, 1.f});
                }
            }
        }
    }

    //
    // voxel editing
    //

    if (state->intersect.t < INFINITY)
    {
        if (mouse_button_down(button::left))
        {
            const u8* kb = SDL_GetKeyboardState(0);

            if (kb[SDL_SCANCODE_LSHIFT])
            {
                int3 p = state->intersect.voxel_coords;
                if (glm::all(glm::greaterThanEqual(p, int3{0})) &&
                    glm::all(glm::lessThan(p, int3{VX_GRID_SIZE})))
                {
                    i32 i = p.x + p.y * VX_GRID_SIZE + p.z * VX_GRID_SIZE * VX_GRID_SIZE;
                    state->voxel_grid[i].flags = 0;
                }
            }
            else
            {
                int3 p = state->intersect.voxel_coords + (int3)state->intersect.normal;

                if (glm::all(glm::greaterThanEqual(p, int3{0})) &&
                    glm::all(glm::lessThan(p, int3{VX_GRID_SIZE})))
                {
                    fprintf(stdout, "Inserted voxel at %d %d %d\n", p.x, p.y, p.z);
                    i32 i = p.x + p.y * VX_GRID_SIZE + p.z * VX_GRID_SIZE * VX_GRID_SIZE;
                    state->voxel_grid[i].flags |= voxel_flag_solid;
                    state->voxel_grid[i].color = state->color_wheel_rgb;
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

namespace
{
void render_wire_cube(const voxed_state* vox_app, const float3& color, const float4x4& model_matrix)
{
    glUseProgram(vox_app->line_shader);
    glUniform3f(2, color.r, color.g, color.b);
    glUniformMatrix4fv(1, 1, GL_FALSE, (float*)&model_matrix);

    glBindVertexArray(vox_app->wire_cube.vao);
    glDrawElements(GL_LINES, vox_app->wire_cube.index_count, GL_UNSIGNED_INT, 0);
}

void render_solid_cube(
    const voxed_state* vox_app,
    const float3& color,
    const float4x4& model_matrix)
{
    glUseProgram(vox_app->solid_shader);
    glUniform3f(2, color.r, color.g, color.b);
    glUniformMatrix4fv(1, 1, GL_FALSE, (float*)&model_matrix);

    glBindVertexArray(vox_app->solid_cube.vao);
    glDrawElements(GL_TRIANGLES, vox_app->solid_cube.index_count, GL_UNSIGNED_INT, 0);
}

void render_voxel_grid_lines(
    const voxed_state* vox_app,
    const float3& color,
    const float4x4& grid_matrix)
{
    glUseProgram(vox_app->line_shader);
    glUniform3f(2, color.r, color.g, color.b);

    float3 translations[3] = {float3(0.f), float3(0.f), float3(0.f)};

    for (int i = 0; i < axis_plane_count; i++)
    {
        float3 n(0.f);
        // the following code pertains to right-handed coordinate systems
        n[(i + 2) % 3] = 1.f;
        float3 forward_dir = forward(vox_app->camera);

        // if we're not facing the positive side of the plane, then we want to
        // translate the grid ruler to the negative side of the plane
        if (glm::dot(forward_dir, n) > 0.f)
        {
            const float plane_translation = -vox_app->voxel_extents.s * (VX_GRID_SIZE / 2);
            translations[(i + 2) % 3][(i + 2) % 3] = plane_translation;
            translations[(i + 1) % 3][(i + 2) % 3] = plane_translation;
        }
    }
    for (int i = 0; i < axis_plane_count; i++)
    {

        float4x4 model_matrix = glm::translate(grid_matrix, translations[i]);
        glUniformMatrix4fv(1, 1, GL_FALSE, (float*)&model_matrix);

        const voxed_state::mesh& mesh = vox_app->voxel_grid_rulers[i];
        glBindVertexArray(mesh.vao);
        glDrawArrays(GL_LINES, 0, mesh.index_count);
    }
}

void render_textured_quad(const voxed_state* vox_app, u32 texture, const float4x4& model_matrix)
{
    glUseProgram(vox_app->textured_shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(2, 0);
    glUniformMatrix4fv(1, 1, GL_FALSE, (float*)&model_matrix);

    glBindVertexArray(vox_app->quad.vao);
    glDrawElements(GL_TRIANGLES, vox_app->quad.index_count, GL_UNSIGNED_INT, 0);
}
}

void voxed_render(voxed_state* state)
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
        u32 shaders[] = {state->line_shader, state->solid_shader, state->textured_shader};
        for (int i = 0; i < vx_countof(shaders); i++)
        {
            glUseProgram(shaders[i]);
            glUniformMatrix4fv(0, 1, GL_FALSE, (float*)&state->camera.transform);
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
                if (state->voxel_grid[i].flags & voxel_flag_solid)
                {
                    float3 voxel_coords{x, y, z};
                    bounds3f voxel_bounds;
                    voxel_bounds.min =
                        state->scene_bounds.min + voxel_coords * state->voxel_extents;
                    voxel_bounds.max = voxel_bounds.min + state->voxel_extents;

                    float4x4 model = glm::translate(float4x4{1.f}, center(voxel_bounds)) *
                                     glm::scale(float4x4{1.f}, 0.5f * state->voxel_extents);

                    render_solid_cube(state, state->voxel_grid[i].color, model);
                }
            }

    //
    // voxel cursor
    //

    if (state->intersect.t < INFINITY)
    {
        render_solid_cube(
            state,
            state->selection_color,
            glm::translate(float4x4{1.f}, state->intersect.position) *
                glm::scale(float4x4{1.f}, float3{0.005f}));

        const u8* kb = SDL_GetKeyboardState(0);

        if (kb[SDL_SCANCODE_LSHIFT])
        {
            render_wire_cube(
                state,
                state->erase_color,
                glm::translate(
                    float4x4{1.f},
                    center(reconstruct_voxel_bounds(
                        state->intersect.voxel_coords, state->scene_bounds, VX_GRID_SIZE))) *
                    glm::scale(float4x4{1.f}, 0.5f * state->voxel_extents));
        }
        else
        {
            render_wire_cube(
                state,
                state->selection_color,
                glm::translate(
                    float4x4{1.f},
                    center(reconstruct_voxel_bounds(
                        state->intersect.voxel_coords, state->scene_bounds, VX_GRID_SIZE)) +
                        state->voxel_extents * state->intersect.normal) *
                    glm::scale(float4x4{1.f}, 0.5f * state->voxel_extents));
        }
    }

    //
    // color wheel
    //

    render_textured_quad(
        state,
        state->color_wheel.tex,
        glm::translate(float4x4{1.f}, float3{2.5f, -1.f, 0.f}) *
            glm::rotate(float4x4{1.f}, -(float)M_PI / 2.0f, float3{1.f, 0.f, 0.f}));

    //
    // grid lines
    //

    {
        glDepthMask(GL_FALSE);
        render_voxel_grid_lines(state, state->grid_color, float4x4{});
        render_wire_cube(state, state->cube_color, float4x4{});
        glDepthMask(GL_TRUE);
    }
}

void voxed_quit(voxed_state* state)
{ // TODO(johann): get rid of delete
    delete state;
}
}
