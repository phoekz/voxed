#include "voxed.h"
#include "common/intersection.h"
#include "common/math_utils.h"
#include "common/mouse.h"
#include "common/array.h"
#include "platform/filesystem.h"
#include "integrations/imgui/imgui_sdl.h"

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
    camera.polar = clamp(camera.polar, -float(0.499 * pi), float(0.499 * pi));
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

enum render_flag
{
    RENDER_FLAG_AMBIENT_OCCLUSION = 1 << 0,
    RENDER_FLAG_DIRECTIONAL_LIGHT = 1 << 1,
};

struct voxed_state
{
    struct mesh
    {
        gpu_buffer *vertices, *indices;
        u32 vertex_count, index_count;
    };

    mesh wire_cube, solid_cube, quad;
    mesh voxel_mesh;

    struct ruler
    {
        mesh mesh;
        i32 offset;
        bool enabled;
    } rulers[axis_plane_count];

    struct
    {
        gpu_texture* texture;
        gpu_sampler* sampler;
        float4x4 model;
        gpu_buffer* buffer;
    } color_wheel;

    struct
    {
        float3 color_hsv;
    } brush;

    struct shader
    {
        gpu_shader *vertex, *fragment;
        gpu_pipeline* pipeline;
    } line_shader, voxel_mesh_shader;

    struct global_constants
    {
        struct
        {
            float4x4 camera;
            u32 flags;
        } data;
        gpu_buffer* buffer;
    } global_constants;

    struct wire_cube_constants
    {
        enum
        {
            selection,
            erase,
            scene_bounds,
            count,
        };
        struct data
        {
            float4x4 model;
            float4 color;
        } data[count];
        gpu_buffer* buffer;
    } wire_cube_constants;

    struct voxel_grid_ruler_constants
    {
        struct data
        {
            float4x4 model;
            float4 color;
        } data[axis_plane_count];
        gpu_buffer* buffer;
    } voxel_grid_ruler_constants;

    struct
    {
        float4 cube{0.1f, 0.1f, 0.1f, 1.0f};
        float4 grid{0.4f, 0.4f, 0.4f, 1.0f};
        float4 selection{0.05f, 0.05f, 0.05f, 1.0f};
        float4 erase{1.0f, 0.1f, 0.1f, 1.0f};
    } colors;
    float4x4 camera_view{};
    orbit_camera camera{};

    bounds3f scene_bounds{float3{-1.f}, float3{1.f}};
    float3 scene_extents;
    float3 voxel_extents;
    voxel_leaf voxel_grid[VX_GRID_SIZE * VX_GRID_SIZE * VX_GRID_SIZE];
    bool voxel_grid_is_dirty;

    voxel_intersect_event intersect;

    struct
    {
        u32 voxel_solid, voxel_empty;
    } stats;

    u32 render_flags;

    voxed_mode mode;
};

voxed_state* voxed_create(platform* platform)
{
    // TODO(johann): figure out a better way to set default values than calling the constructor
    voxed_state* state = new voxed_state();
    gpu_device* gpu = platform->gpu;

    //
    // defaults
    //

    {
        state->render_flags |= RENDER_FLAG_AMBIENT_OCCLUSION;
        state->render_flags |= RENDER_FLAG_DIRECTIONAL_LIGHT;
    }

    //
    // voxel grid
    //

    {
        memset(state->voxel_grid, 0, sizeof(state->voxel_grid));
        state->scene_extents = extents(state->scene_bounds);
        state->voxel_extents = state->scene_extents / (float)VX_GRID_SIZE;
        state->voxel_grid_is_dirty = false;
    }

    //
    // wire cube
    //

    {
        float3 mn{-1.0f}, mx{+1.0f};

        // clang-format off
        float3 vertices[] = {
            {mn.x, mn.y, mn.z}, {mn.x, mn.y, mx.z}, {mx.x, mn.y, mx.z}, {mx.x, mn.y, mn.z},
            {mn.x, mx.y, mn.z}, {mn.x, mx.y, mx.z}, {mx.x, mx.y, mx.z}, {mx.x, mx.y, mn.z},
        };
        uint2 indices[] = {
            {0, 1}, {1, 2}, {2, 3}, {3, 0},
            {4, 5}, {5, 6}, {6, 7}, {7, 4},
            {0, 4}, {1, 5}, {2, 6}, {3, 7},
        };
        // clang-format on

        voxed_state::mesh& m = state->wire_cube;

        m.vertices = gpu_buffer_create(gpu, sizeof(vertices), gpu_buffer_type::vertex);
        m.indices = gpu_buffer_create(gpu, sizeof(indices), gpu_buffer_type::index);

        gpu_buffer_update(gpu, m.vertices, vertices, sizeof(vertices), 0);
        gpu_buffer_update(gpu, m.indices, indices, sizeof(indices), 0);

        state->wire_cube.vertex_count = vx_countof(vertices);
        state->wire_cube.index_count = 2 * vx_countof(indices);
    }

    //
    // solid cube
    //

    {
        float3 mn{-1.0f}, mx{+1.0f};

        struct vertex
        {
            float3 position, normal;
        };

        // clang-format off
        vertex vertices[] =
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
        int3 indices[] =
        {
            {0,1,2}, {2,3,0}, // -x
            {4,5,6}, {6,7,4}, // +x
            {8,9,10}, {10,11,8}, // -y
            {12,13,14}, {14,15,12}, // +y
            {16,17,18}, {18,19,16}, // -z
            {20,21,22}, {22,23,20}, // +z
        };
        // clang-format on

        voxed_state::mesh& m = state->solid_cube;

        m.vertices = gpu_buffer_create(gpu, sizeof(vertices), gpu_buffer_type::vertex);
        m.indices = gpu_buffer_create(gpu, sizeof(indices), gpu_buffer_type::index);

        gpu_buffer_update(gpu, m.vertices, vertices, sizeof(vertices), 0);
        gpu_buffer_update(gpu, m.indices, indices, sizeof(indices), 0);

        state->solid_cube.vertex_count = vx_countof(vertices);
        state->solid_cube.index_count = 3 * vx_countof(indices);
    }

    //
    // voxel rulers
    //

    {
        // defaults

        state->rulers[axis_plane_xy].enabled = false;
        state->rulers[axis_plane_yz].enabled = false;
        state->rulers[axis_plane_zx].enabled = true;

        state->rulers[axis_plane_xy].offset = 0;
        state->rulers[axis_plane_yz].offset = 0;
        state->rulers[axis_plane_zx].offset = -VX_GRID_SIZE / 2;

        struct line
        {
            float3 a{0.f}, b{0.f};
        };

        // assume the extents are the same in all directions
        const float voxel_extent = state->voxel_extents.x;
        const int line_count = VX_GRID_SIZE + 1;
        const float half_line_length = 0.5f * VX_GRID_SIZE * voxel_extent;

        array<line> grid_lines(axis_plane_count * 2 * line_count);

        for (int i = 0; i < axis_plane_count; i++)
        {
            for (int j = 0; j < line_count; j++)
            {
                float line_offset = -half_line_length + j * voxel_extent;

                line& l0 = grid_lines.add();
                l0.a[i] = -half_line_length;
                l0.b[i] = +half_line_length;
                l0.a[(i + 1) % 3] = line_offset;
                l0.b[(i + 1) % 3] = line_offset;

                line& l1 = grid_lines.add();
                l1.a[i] = line_offset;
                l1.b[i] = line_offset;
                l1.a[(i + 1) % 3] = -half_line_length;
                l1.b[(i + 1) % 3] = +half_line_length;
            }

            voxed_state::mesh& m = state->rulers[i].mesh;
            m.vertex_count = m.index_count = 2 * (u32)grid_lines.size();

            m.vertices = gpu_buffer_create(gpu, grid_lines.byte_size(), gpu_buffer_type::vertex);
            gpu_buffer_update(gpu, m.vertices, grid_lines.ptr(), grid_lines.byte_size(), 0);

            grid_lines.clear();
        }
    }

    //
    // quad
    //

    {
        struct vertex
        {
            float3 position;
            float2 uv;
        };
        vertex vertices[] = {
            {{-1.f, -1.f, 0.f}, {0.f, 0.f}},
            {{+1.f, -1.f, 0.f}, {1.f, 0.f}},
            {{+1.f, +1.f, 0.f}, {1.f, 1.f}},
            {{-1.f, +1.f, 0.f}, {0.f, 1.f}},
        };
        int3 indices[] = {{0, 1, 2}, {2, 3, 0}};

        voxed_state::mesh& m = state->quad;

        m.vertices = gpu_buffer_create(gpu, sizeof(vertices), gpu_buffer_type::vertex);
        m.indices = gpu_buffer_create(gpu, sizeof(indices), gpu_buffer_type::index);

        gpu_buffer_update(gpu, m.vertices, vertices, sizeof(vertices), 0);
        gpu_buffer_update(gpu, m.indices, indices, sizeof(indices), 0);

        state->quad.vertex_count = vx_countof(vertices);
        state->quad.index_count = 3 * vx_countof(indices);
    }

    //
    // color wheel
    //

    {
        // Defaults

        state->brush.color_hsv = float3(0.0f, 0.0f, 1.0f); // white

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
                float r = glm::length(p);
                if (r < 1.f)
                {
                    float angle = remap_range(glm::atan(p.y, p.x), -float(pi), float(pi), 0.f, 1.f);
                    float3 hsv = float3(angle, r, 1.f);
                    float3 rgb = hsv_to_rgb(hsv);
                    for (int c = 0; c < 3; c++)
                        pixels[4 * i + c] = (u8)(255.f * rgb[c]);
                    pixels[4 * i + 3] = 255;
                }
                else
                    pixels[4 * i + 3] = 0;
            }

        state->color_wheel.texture =
            gpu_texture_create(gpu, w, h, gpu_pixel_format::rgba8_unorm_srgb, pixels);
        state->color_wheel.sampler = gpu_sampler_create(
            gpu, gpu_filter_mode::linear, gpu_filter_mode::linear, gpu_filter_mode::nearest);

        std::free(pixels);
    }

    //
    // shaders
    //

    {
        gpu_pipeline_options line_opt = {}, voxel_mesh_opt = {};

        line_opt.blend_enabled = false;
        line_opt.culling_enabled = true;
        line_opt.depth_test_enabled = true;
        line_opt.depth_write_enabled = true;

        voxel_mesh_opt.blend_enabled = false;
        voxel_mesh_opt.culling_enabled = true;
        voxel_mesh_opt.depth_test_enabled = true;
        voxel_mesh_opt.depth_write_enabled = true;

#if VX_GRAPHICS_API == VX_GRAPHICS_API_METAL
#define SHADER_PATH(name) "shaders/mtl/" name ".metallib"
#elif VX_GRAPHICS_API == VX_GRAPHICS_API_OPENGL
#define SHADER_PATH(name) "shaders/gl/" name ".glsl"
#endif

        struct
        {
            const char* path;
            voxed_state::shader& shader;
            const gpu_pipeline_options& opt;
        } sources[] = {
            {SHADER_PATH("line"), state->line_shader, line_opt},
            {SHADER_PATH("voxel_mesh"), state->voxel_mesh_shader, voxel_mesh_opt},
        };

#undef SHADER_PATH

        for (int i = 0; i < vx_countof(sources); i++)
        {
            char* program_src;
            usize program_size;

            program_src = read_whole_file(sources[i].path, &program_size);

            voxed_state::shader& shader = sources[i].shader;
            shader.vertex = gpu_shader_create(
                gpu, gpu_shader_type::vertex, program_src, program_size, "vertex_main");
            shader.fragment = gpu_shader_create(
                gpu, gpu_shader_type::fragment, program_src, program_size, "fragment_main");
            shader.pipeline =
                gpu_pipeline_create(gpu, shader.vertex, shader.fragment, sources[i].opt);
            free(program_src);
        }

        state->global_constants.buffer =
            gpu_buffer_create(gpu, sizeof(state->global_constants.data), gpu_buffer_type::constant);
        state->wire_cube_constants.buffer = gpu_buffer_create(
            gpu, sizeof(state->wire_cube_constants.data), gpu_buffer_type::constant);
        state->voxel_grid_ruler_constants.buffer = gpu_buffer_create(
            gpu, sizeof(state->voxel_grid_ruler_constants.data), gpu_buffer_type::constant);
        state->color_wheel.buffer =
            gpu_buffer_create(gpu, sizeof(state->color_wheel.model), gpu_buffer_type::constant);
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
        const u8* kb = SDL_GetKeyboardState(0);

        if (mouse_button_pressed(button::right) || kb[SDL_SCANCODE_SPACE])
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
            const auto& ruler = state->rulers[axis];

            if (!ruler.enabled)
                continue;

            // NOTE(vinht): Having a tiny extent in the third dimension
            // improves numerical precision in the ray-test. Remapping from
            // intersection point to voxel grid can be done more robustly (no
            // need to handle negative zeroes).
            const float tiny_extent = 1.0f / (1 << 16);
            float t;
            float3 mn, mx;
            mn = state->scene_bounds.min;
            mx = state->scene_bounds.max;
            float ext = state->voxel_extents.x;
            mn[(axis + 2) % 3] = -tiny_extent + ext * ruler.offset;
            mx[(axis + 2) % 3] = +tiny_extent + ext * ruler.offset;

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
                    fprintf(stdout, "Erased voxel from %d %d %d\n", p.x, p.y, p.z);
                    i32 i = p.x + p.y * VX_GRID_SIZE + p.z * VX_GRID_SIZE * VX_GRID_SIZE;
                    state->voxel_grid[i].flags = 0;
                    state->voxel_grid_is_dirty = true;
                }
            }
            else
            {
                int3 p = state->intersect.voxel_coords + (int3)state->intersect.normal;

                if (glm::all(glm::greaterThanEqual(p, int3{0})) &&
                    glm::all(glm::lessThan(p, int3{VX_GRID_SIZE})))
                {
                    fprintf(stdout, "Placed voxel at %d %d %d\n", p.x, p.y, p.z);
                    i32 i = p.x + p.y * VX_GRID_SIZE + p.z * VX_GRID_SIZE * VX_GRID_SIZE;
                    state->voxel_grid[i].flags |= voxel_flag_solid;
                    state->voxel_grid[i].color = hsv_to_rgb(state->brush.color_hsv);
                    state->voxel_grid_is_dirty = true;
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

    //
    // voxel statistics
    //

    if (state->voxel_grid_is_dirty)
    {
        state->stats.voxel_solid = state->stats.voxel_empty = 0;

        for (int z = 0; z < VX_GRID_SIZE; z++)
            for (int y = 0; y < VX_GRID_SIZE; y++)
                for (int x = 0; x < VX_GRID_SIZE; x++)
                {
                    int i = x + y * VX_GRID_SIZE + z * VX_GRID_SIZE * VX_GRID_SIZE;
                    if (state->voxel_grid[i].flags & voxel_flag_solid)
                        state->stats.voxel_solid++;
                    else
                        state->stats.voxel_empty++;
                }
    }
}

void voxed_gui(voxed_state* state)
{
    static bool hack_instant_load = false;
    ImGui::Begin("voxed");
    ImGui::Value("Resolution", VX_GRID_SIZE);
    ImGui::Separator();
    ImGui::Value("Voxel Leaf Bytes", (int)sizeof(voxel_leaf));
    ImGui::Value("Voxel Grid Bytes", (int)sizeof(state->voxel_grid));
    ImGui::Separator();
    ImGui::Value("Total Voxels", pow3(VX_GRID_SIZE));
    ImGui::Value("Empty Voxels", state->stats.voxel_empty);
    ImGui::Value("Solid Voxels", state->stats.voxel_solid);
    ImGui::Separator();
    ImGui::Value("Vertices", state->voxel_mesh.vertex_count);
    ImGui::Value("Triangles", state->voxel_mesh.index_count / 3);
    ImGui::Separator();
    ImGui::CheckboxFlags("Ambient Occlusion", &state->render_flags, RENDER_FLAG_AMBIENT_OCCLUSION);
    ImGui::CheckboxFlags("Directional Light", &state->render_flags, RENDER_FLAG_DIRECTIONAL_LIGHT);
    ImGui::Separator();
    if (ImGui::Button("Save"))
    {
        if (FILE* f = fopen("scene.vx", "wb"))
        {
            fwrite(state->voxel_grid, sizeof state->voxel_grid, 1, f);
            fclose(f);
            fprintf(stdout, "Saved scene\n");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Load") || hack_instant_load)
    {
        if (FILE* f = fopen("scene.vx", "rb"))
        {
            fread(state->voxel_grid, sizeof state->voxel_grid, 1, f);
            fclose(f);
            fprintf(stdout, "Loaded scene\n");
            state->voxel_grid_is_dirty = true;
        }
        hack_instant_load = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear"))
    {
        memset(state->voxel_grid, 0, sizeof(state->voxel_grid));
        state->voxel_grid_is_dirty = true;
    }
    ImGui::Separator();
    ImGui::Text("Rulers");
    static const char* plane_names[] = {"XY", "YZ", "ZX"};
    for (int i = 0; i < axis_plane_count; ++i)
    {
        voxed_state::ruler& ruler = state->rulers[i];
        ImGui::PushID(i);
        ImGui::Text("%s", plane_names[i]);
        ImGui::SameLine();
        ImGui::SliderInt("##slider", &ruler.offset, -VX_GRID_SIZE / 2, VX_GRID_SIZE / 2);
        ImGui::SameLine();
        ImGui::Checkbox("##checkbox", &ruler.enabled);
        ImGui::PopID();
    }
    ImGui::Separator();
    ImGui::Text("Color Wheel");
    {
        const float wheel_diameter = 200.0f /* pixels */;

        // draw color wheel texture while caching key locations & sizes
        ImVec2 pre = ImGui::GetCursorPos();
        ImVec2 anchor = ImGui::GetCursorScreenPos();
        ImGui::Image(state->color_wheel.texture, ImVec2(wheel_diameter, wheel_diameter));
        ImVec2 rmin = ImGui::GetItemRectMin();
        ImVec2 rmax = ImGui::GetItemRectMax();
        ImVec2 post = ImGui::GetCursorPos();

        // relative mouse position
        ImVec2 mouse_pos = ImGui::GetMousePos();
        ImVec2 rel_pos = ImVec2(mouse_pos.x - rmin.x, mouse_pos.y - rmin.y);

        // normalized mouse position [-1, 1]
        float normx, normy;
        normx = remap_range(rel_pos.x, 0.0f, wheel_diameter, -1.0f, 1.0f);
        normy = remap_range(rel_pos.y, 0.0f, wheel_diameter, -1.0f, 1.0f);
        float dist_to_origin = glm::length(float2(normx, normy));

        // update color value when a click has been detected within the wheel
        if (mouse_button_pressed(button::left) && dist_to_origin < 1.0f)
        {
            float angle = remap_range(glm::atan(normy, normx), -float(pi), float(pi), 0.f, 1.f);
            state->brush.color_hsv = float3(angle, dist_to_origin, 1.0f);
        }

        // absolute position of the mouse cursor
        ImVec2 hovering_pos = anchor;
        hovering_pos.x += rel_pos.x, hovering_pos.y += rel_pos.y;

        // absolute position of the selected color
        ImVec2 selected_pos = anchor;
        selected_pos.x += 0.5f * (rmax.x - rmin.x);
        selected_pos.y += 0.5f * (rmax.y - rmin.y);
        float3 hsv = state->brush.color_hsv;
        float angle = remap_range(hsv.x, 0.0f, 1.0f, -pi, pi);
        selected_pos.x += 0.5f * wheel_diameter * hsv.y * std::cos(angle);
        selected_pos.y += 0.5f * wheel_diameter * hsv.y * std::sin(angle);

        // render both hovering color and selected color
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImColor selected_color = ImColor(0.05f, 0.05f, 0.05f, 1.0f);
        ImColor hovering_color = ImColor(0.25f, 0.25f, 0.25f, 1.0f);
        ImGui::SetCursorPos(pre);
        if (ImGui::IsItemHovered() && dist_to_origin < 1.0f)
            draw_list->AddCircle(hovering_pos, 4.0f, hovering_color, 12, 2.0f);
        draw_list->AddCircle(selected_pos, 4.0f, selected_color, 12, 2.0f);
        ImGui::SetCursorPos(post);
    }
    ImGui::End();
}

void voxed_gpu_update(voxed_state* state, gpu_device* gpu)
{
    //
    // setup constants
    //

    // globals

    {
        state->global_constants.data.camera = state->camera.transform;
        state->global_constants.data.flags = state->render_flags;
    }

    // voxel rulers

    {
        for (int i = 0; i < axis_plane_count; ++i)
        {
            auto& pl = state->voxel_grid_ruler_constants.data[i];
            auto& ruler = state->rulers[i];
            float3 offset{0.0f};
            offset[(i + 2) % 3] = ruler.offset * state->voxel_extents.x;
            pl.model = glm::translate(float4x4{}, offset);
            pl.color = state->colors.grid;
        }
    }

    // voxel cursors

    {
        float4x4 voxel_xform = {};

        if (state->intersect.t < INFINITY)
        {
            voxel_xform =
                glm::translate(
                    float4x4{1.f},
                    center(reconstruct_voxel_bounds(
                        state->intersect.voxel_coords, state->scene_bounds, VX_GRID_SIZE)) +
                        state->voxel_extents * state->intersect.normal) *
                glm::scale(float4x4{1.f}, 0.5f * state->voxel_extents);
        }

        auto& sel = state->wire_cube_constants.data[voxed_state::wire_cube_constants::selection];
        sel.model = voxel_xform;
        sel.color = state->colors.selection;

        auto& era = state->wire_cube_constants.data[voxed_state::wire_cube_constants::erase];
        era.model = voxel_xform;
        era.color = state->colors.erase;

        auto& scb = state->wire_cube_constants.data[voxed_state::wire_cube_constants::scene_bounds];
        scb.model = float4x4{};
        scb.color = state->colors.cube;
    }

    // voxel (mesh)

    if (state->voxel_grid_is_dirty)
    {
        fprintf(stdout, "(Re)generating voxel mesh\n");

        struct vertex
        {
            float3 pos;
            u32 rgba;
            float3 nm;
            float ao;
        };

        array<vertex> new_vbo;
        array<int3> new_ibo;

        // for each solid voxel:
        //   for each face:
        //     if neighbor is empty or out of bounds:
        //       add face quad vertices and indices

        for (int z = 0; z < VX_GRID_SIZE; z++)
            for (int y = 0; y < VX_GRID_SIZE; y++)
                for (int x = 0; x < VX_GRID_SIZE; x++)
                {
                    int ci = pows3(x, y, z, VX_GRID_SIZE);
                    int3 c = int3(x, y, z);
                    const voxel_leaf& ivx = state->voxel_grid[ci];

                    // only process solid voxels
                    if (~ivx.flags & voxel_flag_solid)
                        continue;

                    for (int di = 0; di < 6; di++)
                    {
                        bool make_face = false;
                        int3 d(0), n(0);

                        d[di % 3] = di / 3 ? -1 : 1;
                        n = c + d;

                        if (n.x >= 0 && n.y >= 0 && n.z >= 0 && n.x < VX_GRID_SIZE &&
                            n.y < VX_GRID_SIZE && n.z < VX_GRID_SIZE)
                        {
                            int ni = pows3(n.x, n.y, n.z, VX_GRID_SIZE);
                            const voxel_leaf& nvx = state->voxel_grid[ni];
                            if (~nvx.flags & voxel_flag_solid)
                                make_face = true;
                        }
                        else
                            make_face = true;

                        if (make_face)
                        {
                            const bounds3f& sb = state->scene_bounds;
                            float3 color = ivx.color;
                            vertex va, vb, vc, vd;

                            // normal
                            float3 nm(0.0f);
                            nm[di % 3] = float(d[di % 3]);
                            va.nm = vb.nm = vc.nm = vd.nm = nm;

                            // position
                            //   place 4 vertices to the voxel midpoint
                            //   move vertices along the face normal
                            //   move vertices to one of the face corners
                            float half_ext = 0.5f * state->voxel_extents.x;
                            float3 center(
                                (x + 0.5f) / VX_GRID_SIZE,
                                (y + 0.5f) / VX_GRID_SIZE,
                                (z + 0.5f) / VX_GRID_SIZE);
                            float3 pos;
                            pos = glm::mix(sb.min, sb.max, center);
                            pos += half_ext * nm;
                            float3 fca(0.0f), fcb(0.0f), fcc(0.0f), fcd(0.0f);
                            fca[(di + 1) % 3] = -half_ext, fca[(di + 2) % 3] = -half_ext;
                            fcb[(di + 1) % 3] = +half_ext, fcb[(di + 2) % 3] = -half_ext;
                            fcc[(di + 1) % 3] = +half_ext, fcc[(di + 2) % 3] = +half_ext;
                            fcd[(di + 1) % 3] = -half_ext, fcd[(di + 2) % 3] = +half_ext;
                            va.pos = pos + fca;
                            vb.pos = pos + fcb;
                            vc.pos = pos + fcc;
                            vd.pos = pos + fcd;

                            // color
                            u32 rgba = 0;
                            rgba |= u8(color.r * 0xFF) << 0;
                            rgba |= u8(color.g * 0xFF) << 8;
                            rgba |= u8(color.b * 0xFF) << 16;
                            rgba |= 0xFF << 24;
                            va.rgba = vb.rgba = vc.rgba = vd.rgba = rgba;

                            // ambient occlusion
                            //
                            // Credits to:
                            // https://0fps.net/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/
                            //
                            // Search all the s's around x in the normal
                            // direction.
                            //
                            //   top view   side views
                            //   [s][s][s]  [s][s][s]  [s][s][s]  [s][s][s]  [s][s][s]
                            //   [s][x][s]     [x]        [x]        [x]        [x]
                            //   [s][s][s]
                            //
                            //   [7][6][5]  [1][2][3]  [3][4][5]  [5][6][7]  [7][0][1]
                            //   [0][x][4]     [x]        [x]        [x]        [x]
                            //   [1][2][3]
                            //
                            // With this layout we can mask 3 bits at the time
                            // in offsets of two like so:
                            //
                            //   012345670
                            //   aaa||||||
                            //     bbb||||
                            //       ccc||
                            //         ddd

                            // clang-format off
                            static const int2 search_dirs[] =
                            {
                                int2(-1, +0), // 0
                                int2(-1, -1), // 1
                                int2(+0, -1), // 2
                                int2(+1, -1), // 3
                                int2(+1, +0), // 4
                                int2(+1, +1), // 5
                                int2(+0, +1), // 6
                                int2(-1, +1), // 7
                            };
                            static const float symmetries[] =
                            {
                                0.0f, // case 0 - none
                                0.5f, // case 1 - edge
                                0.5f, // case 2 - edge + corner
                                1.0f, // case 3 - corner
                            };
                            // clang-format on

                            u32 mask = 0;

                            for (int search_dir = 0; search_dir < 8; search_dir++)
                            {
                                int3 s;
                                s[di % 3] = n[di % 3];
                                s[(di + 1) % 3] = n[(di + 1) % 3] + search_dirs[search_dir][0];
                                s[(di + 2) % 3] = n[(di + 2) % 3] + search_dirs[search_dir][1];

                                if (s.x >= 0 && s.y >= 0 && s.z >= 0 && s.x < VX_GRID_SIZE &&
                                    s.y < VX_GRID_SIZE && s.z < VX_GRID_SIZE)
                                {
                                    int si = pows3(s.x, s.y, s.z, VX_GRID_SIZE);
                                    const voxel_leaf& svx = state->voxel_grid[si];
                                    if (svx.flags & voxel_flag_solid)
                                        mask |= 1 << search_dir;
                                }
                            }

                            // NOTE(vinht): Trick, copy 0th bit to 8th bit, so
                            // we can mask out 3 bits per corner without
                            // special cases.
                            mask |= (mask & 0x1) << 8;

                            u32 maska = (mask >> 0) & 0x7;
                            u32 maskb = (mask >> 2) & 0x7;
                            u32 maskc = (mask >> 4) & 0x7;
                            u32 maskd = (mask >> 6) & 0x7;
                            u32 cnta = vx_popcnt(maska);
                            u32 cntb = vx_popcnt(maskb);
                            u32 cntc = vx_popcnt(maskc);
                            u32 cntd = vx_popcnt(maskd);
                            va.ao = maska == 0x5 ? symmetries[3] : symmetries[cnta];
                            vb.ao = maskb == 0x5 ? symmetries[3] : symmetries[cntb];
                            vc.ao = maskc == 0x5 ? symmetries[3] : symmetries[cntc];
                            vd.ao = maskd == 0x5 ? symmetries[3] : symmetries[cntd];

                            // indices
                            int idx = new_vbo.size();
                            int3 ta, tb;
                            // NOTE(vinht): Flip triangulation based on sum of
                            // AO values of the two diagonals to avoid
                            // interpolation artifacts.
                            if (va.ao + vc.ao < vb.ao + vd.ao)
                            {
                                ta = int3(idx + 0, idx + 1, idx + 2);
                                tb = int3(idx + 0, idx + 2, idx + 3);
                            }
                            else
                            {
                                ta = int3(idx + 0, idx + 1, idx + 3);
                                tb = int3(idx + 1, idx + 2, idx + 3);
                            }
                            // flip winding
                            if (glm::dot(nm, float3(1.0f)) < 0.0f)
                                std::swap(ta.y, ta.z), std::swap(tb.y, tb.z);

                            new_vbo.add(va), new_vbo.add(vb), new_vbo.add(vc), new_vbo.add(vd);
                            new_ibo.add(ta), new_ibo.add(tb);
                        }
                    }
                }

        voxed_state::mesh& m = state->voxel_mesh;

        if (m.vertices)
            gpu_buffer_destroy(gpu, m.vertices);
        if (m.indices)
            gpu_buffer_destroy(gpu, m.indices);

        m.vertices = gpu_buffer_create(gpu, new_vbo.byte_size(), gpu_buffer_type::vertex);
        m.indices = gpu_buffer_create(gpu, new_ibo.byte_size(), gpu_buffer_type::index);

        gpu_buffer_update(gpu, m.vertices, new_vbo.ptr(), new_vbo.byte_size(), 0);
        gpu_buffer_update(gpu, m.indices, new_ibo.ptr(), new_ibo.byte_size(), 0);

        m.vertex_count = new_vbo.size();
        m.index_count = 3 * new_ibo.size();

        state->voxel_grid_is_dirty = false;
    }

    //
    // buffer updates
    //

    {
        gpu_buffer_update(
            gpu,
            state->voxel_grid_ruler_constants.buffer,
            state->voxel_grid_ruler_constants.data,
            sizeof(state->voxel_grid_ruler_constants.data),
            0);

        gpu_buffer_update(
            gpu,
            state->global_constants.buffer,
            &state->global_constants.data,
            sizeof(state->global_constants.data),
            0);

        gpu_buffer_update(
            gpu,
            state->wire_cube_constants.buffer,
            state->wire_cube_constants.data,
            sizeof(state->wire_cube_constants.data),
            0);

        gpu_buffer_update(
            gpu,
            state->color_wheel.buffer,
            &state->color_wheel.model,
            sizeof(state->color_wheel.model),
            0);
    }
}

void voxed_gpu_draw(voxed_state* state, gpu_channel* channel)
{
    //
    // command submission
    //

    // voxel rulers

    {
        gpu_channel_set_pipeline_cmd(channel, state->line_shader.pipeline);
        gpu_channel_set_buffer_cmd(channel, state->global_constants.buffer, 1);
        gpu_channel_set_buffer_cmd(channel, state->voxel_grid_ruler_constants.buffer, 2);

        for (int i = 0; i < axis_plane_count; ++i)
        {
            const auto& ruler = state->rulers[i];

            if (!ruler.enabled)
                continue;

            const voxed_state::mesh& mesh = ruler.mesh;
            gpu_channel_set_buffer_cmd(channel, mesh.vertices, 0);
            gpu_channel_draw_primitives_cmd(
                channel, gpu_primitive_type::line, 0, mesh.vertex_count, 1, i);
        }
    }

    // voxel cursors

    if (state->intersect.t < INFINITY)
    {
        gpu_channel_set_pipeline_cmd(channel, state->line_shader.pipeline);
        gpu_channel_set_buffer_cmd(channel, state->wire_cube.vertices, 0);
        gpu_channel_set_buffer_cmd(channel, state->global_constants.buffer, 1);
        gpu_channel_set_buffer_cmd(channel, state->wire_cube_constants.buffer, 2);

        const u8* kb = SDL_GetKeyboardState(0);

        if (kb[SDL_SCANCODE_LSHIFT])
        {
            gpu_channel_draw_indexed_primitives_cmd(
                channel,
                gpu_primitive_type::line,
                state->wire_cube.index_count,
                gpu_index_type::u32,
                state->wire_cube.indices,
                0,
                1,
                0,
                voxed_state::wire_cube_constants::erase);
        }
        else
        {
            gpu_channel_draw_indexed_primitives_cmd(
                channel,
                gpu_primitive_type::line,
                state->wire_cube.index_count,
                gpu_index_type::u32,
                state->wire_cube.indices,
                0,
                1,
                0,
                voxed_state::wire_cube_constants::selection);
        }
    }

    // scene bounds

    {
        gpu_channel_set_pipeline_cmd(channel, state->line_shader.pipeline);
        gpu_channel_set_buffer_cmd(channel, state->wire_cube.vertices, 0);
        gpu_channel_set_buffer_cmd(channel, state->global_constants.buffer, 1);
        gpu_channel_set_buffer_cmd(channel, state->wire_cube_constants.buffer, 2);
        gpu_channel_draw_indexed_primitives_cmd(
            channel,
            gpu_primitive_type::line,
            state->wire_cube.index_count,
            gpu_index_type::u32,
            state->wire_cube.indices,
            0,
            1,
            0,
            voxed_state::wire_cube_constants::scene_bounds);
    }

    // voxels

    if (state->voxel_mesh.vertex_count)
    {
        gpu_channel_set_pipeline_cmd(channel, state->voxel_mesh_shader.pipeline);
        gpu_channel_set_buffer_cmd(channel, state->voxel_mesh.vertices, 0);
        gpu_channel_set_buffer_cmd(channel, state->global_constants.buffer, 1);
        gpu_channel_draw_indexed_primitives_cmd(
            channel,
            gpu_primitive_type::triangle,
            state->voxel_mesh.index_count,
            gpu_index_type::u32,
            state->voxel_mesh.indices,
            0,
            1,
            0,
            0);
    }
}

void voxed_quit(voxed_state* state)
{
    // TODO(johann): get rid of delete
    delete state;
}
}
