#include "orbit_camera.h"
#include "common/base.h"
#include "common/math_utils.h"
#include "common/mouse.h"

#include <assert.h>
#include <stdlib.h>

namespace vx
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

    bool initialized{false};
};

static orbit_camera _camera;

// The coordinate system used here is the following.
// The polar is the angle between the z-axis and the y-axis
// The azimuth is the angle between the z-axis and the x-axis.
static float3 forward(const orbit_camera& camera)
{
    const float cos_polar = std::cos(camera.polar);
    return glm::normalize(float3(
        -cos_polar * std::sin(camera.azimuth),
        -std::sin(camera.polar),
        -cos_polar * std::cos(camera.azimuth)));
}

static float3 right(const orbit_camera& camera)
{
    const float cos_polar = std::cos(camera.polar);
    float z = cos_polar * std::cos(camera.azimuth);
    float x = cos_polar * std::sin(camera.azimuth);
    return glm::normalize(float3(z, 0.f, -x));
}

static float3 up(const orbit_camera& camera) { return glm::cross(right(camera), forward(camera)); }

static float3 eye(const orbit_camera& camera)
{
    return camera.focal_point - camera.radius * forward(camera);
}

orbit_camera* orbit_camera_initialize()
{
    assert(!_camera.initialized);
    _camera.initialized = true;
    return &_camera;
}

void orbit_camera_dolly(orbit_camera* camera, float dz, float dt)
{
    const float scale = 8.f;
    // the delta radius is scaled here by the radius itself, so that the scaling acts faster
    // the further away the camera is
    camera->radius -= dt * scale * dz * camera->radius;
    camera->radius = clamp(camera->radius, camera->min_radius, camera->max_radius);
}

void orbit_camera_rotate(orbit_camera* camera, float dx, float dy, float dt)
{
    const float scale = 16.f;
    camera->polar += dt * scale * degrees_to_radians(dy);
    camera->azimuth -= dt * scale * degrees_to_radians(dx);
    camera->polar = clamp(camera->polar, -float(0.499f * pi), float(0.499f * pi));
}

float4x4 orbit_camera_matrix(orbit_camera* camera, int width, int height)
{
    const float aspect_ratio = float(width) / height;
    return glm::perspective(camera->fovy, aspect_ratio, camera->near, camera->far) *
           glm::lookAt(eye(*camera), camera->focal_point, up(*camera));
}

ray orbit_camera_ray(
    orbit_camera* camera,
    int2 screen_coordinates,
    int screen_width,
    int screen_height)
{
    ray ray;
    ray.origin = eye(*camera);

    const float aspect = float(screen_width) / screen_height;
    const float nx = 2.f * (float(screen_coordinates.x) / screen_width - .5f);
    const float ny = -2.f * (float(screen_coordinates.y) / screen_height - .5f);

    float vertical_extent = std::tan(0.5f * camera->fovy) * camera->near;
    ray.direction = forward(*camera) * camera->near;
    ray.direction += aspect * nx * vertical_extent * right(*camera);
    ray.direction += ny * vertical_extent * up(*camera);
    ray.direction = glm::normalize(ray.direction);

    return ray;
}
}
