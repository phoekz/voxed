layout(std430, binding = 0) restrict readonly buffer vertex_t
{
    float vertex_data[];
};

layout(std430, binding = 1, column_major) buffer global_constants
{
    mat4 camera;
} global;

layout(std430, binding = 2, column_major) buffer skybox_constants
{
    mat4 transform;
    vec3 sun_direction;
    float sun_disk_radius;
    vec2 sun_disk_vertical_bounds;
    vec2 starfield_params;
    vec4 bg_color_a;
    vec4 bg_color_b;
    vec4 sun_color_a;
    vec4 sun_color_b;
    int outrun_mode;
} skybox;

struct object_t
{
    mat4 model;
    vec4 color;
};

#if VX_SHADER == VX_VERTEX_SHADER

out vec4 v_pos;

void main()
{
    vec3 pos;
    pos.x = vertex_data[gl_VertexID * 6 + 0];
    pos.y = vertex_data[gl_VertexID * 6 + 1];
    pos.z = vertex_data[gl_VertexID * 6 + 2];

    vec3 nm;
    nm.x = vertex_data[gl_VertexID * 6 + 3];
    nm.y = vertex_data[gl_VertexID * 6 + 4];
    nm.z = vertex_data[gl_VertexID * 6 + 5];

    uint iid = VX_INSTANCE_ID;
    gl_Position = (skybox.transform * vec4(pos, 1.0)).xyww;
    v_pos = vec4(pos, 1.0);
}

#elif VX_SHADER == VX_FRAGMENT_SHADER

in vec4 v_pos;

out vec4 frag_color;

float cubic_in_out(float t) {
  return t < 0.5
    ? 4.0 * t * t * t
    : 0.5 * pow(2.0 * t - 2.0, 3.0) + 1.0;
}

float quartic_in_out(float t) {
  return t < 0.5
    ? 8.0 * pow(t, 4.0)
    : -8.0 * pow(t - 1.0, 4.0) + 1.0;
}

float quintic_in_out(float t) {
  return t < 0.5
    ? +16.0 * pow(t, 5.0)
    : -0.5 * pow(2.0 * t - 2.0, 5.0) + 1.0;
}

float exponential_in_out(float t) {
  return t == 0.0 || t == 1.0
    ? t
    : t < 0.5
      ? +0.5 * pow(2.0, (20.0 * t) - 10.0)
      : -0.5 * pow(2.0, 10.0 - (t * 20.0)) + 1.0;
}

float remap(float x, float a_mn, float a_mx, float b_mn, float b_mx)
{
    return b_mn + (x - a_mn) * (b_mx - b_mn) / (a_mx - a_mn);
}

vec3 hash(vec3 x)
{
    x = vec3(dot(x, vec3(127.1, 311.7,  74.7)),
             dot(x, vec3(269.5, 183.3, 246.1)),
             dot(x, vec3(113.5, 271.9, 124.6)));
    return fract(sin(x) * 43758.5453123);
}

vec3 voronoi(in vec3 x)
{
    vec3 p = floor(x);
    vec3 f = fract(x);

    float id = 0.0;
    vec2 res = vec2(100.0);
    for (int k = -1; k <= 1; k++)
    for (int j = -1; j <= 1; j++)
    for (int i = -1; i <= 1; i++)
    {
        vec3 b = vec3(float(i), float(j), float(k));
        vec3 r = vec3(b) - f + hash(p + b);
        float d = dot(r, r);

        if (d < res.x)
        {
            id = dot(p + b, vec3(1.0, 57.0, 113.0));
            res = vec2(d, res.x);
        }
        else if (d < res.y)
            res.y = d;
    }

    return vec3(sqrt(res), abs(id));
}

void main()
{
    vec3 point_on_sphere = normalize(v_pos.xyz);

    float point_y = 0.5 + 0.5 * point_on_sphere.y;
    frag_color.rgb = mix(skybox.bg_color_a.rgb, skybox.bg_color_b.rgb, exponential_in_out(point_y));

    if (skybox.outrun_mode == 1)
    {
        vec3 star_cell = voronoi(skybox.starfield_params.x * point_on_sphere);
        if (star_cell.x < skybox.starfield_params.y)
            frag_color.rgb = vec3(1.0, 1.0, 1.0);

        float point_to_sun = acos(dot(skybox.sun_direction, point_on_sphere));
        if (point_to_sun < skybox.sun_disk_radius)
        {
            const vec3 up = vec3(0.0, 1.0, 0.0);
            float sun_a = remap(
                dot(up, point_on_sphere),
                skybox.sun_disk_vertical_bounds.x,
                skybox.sun_disk_vertical_bounds.y,
                0.0,
                1.0);

            float stripe = 1.0;
            if (sun_a < 0.5)
                stripe = sin(128.0 * pow(sun_a, 1.2));
            if (stripe > 0.0)
                frag_color.rgb = mix(skybox.sun_color_a.rgb, skybox.sun_color_b.rgb, sun_a);
        }
    }

    frag_color.a = 1.0;
}

#endif
