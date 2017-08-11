layout(std430, binding = 1, column_major) buffer global_constants
{
    mat4 camera;
} global;

struct object_t
{
    vec3 offset;
    float scale;
    vec4 color;
};

layout(std430, binding = 2, column_major) buffer object_constants
{
    object_t objects[];
};

#if VX_SHADER == VX_VERTEX_SHADER

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

out vec3 v_normal;
out vec4 v_color;

void main()
{
    uint iid = VX_INSTANCE_ID;
    vec4 v = vec4(a_position, 1.0);
    v.xyz *= objects[iid].scale;
    v.xyz += objects[iid].offset;

    gl_Position = global.camera * v;
    v_normal = a_normal;
    v_color = objects[iid].color;
}

#elif VX_SHADER == VX_FRAGMENT_SHADER

in vec3 v_normal;
in vec4 v_color;

out vec4 frag_color;

void main()
{
    vec3 n = normalize(v_normal);
    vec3 ld = normalize(vec3(1.0, 2.0, 3.0));
    float l = 0.5 + 0.5 * max(dot(n, ld), 0.0);
    frag_color = vec4(l * v_color.rgb, 1.0);
}

#endif
