#pragma once

#include "common/geometry.h"

namespace vx
{

struct orbit_camera;

orbit_camera* orbit_camera_initialize();

// dolly the camera (move along forward axis) by an amount dz
void orbit_camera_dolly(orbit_camera* camera, float dz, float dt);

void orbit_camera_rotate(orbit_camera* camera, float dx, float dy, float dt);

float4x4 orbit_camera_matrix(orbit_camera* camera, int screen_width, int screen_height);

ray orbit_camera_ray(
    orbit_camera* camera,
    int2 screen_coordinates,
    int screen_width,
    int screen_height);
}
