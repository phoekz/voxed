layout(std430, binding = 1, column_major) buffer global_constants
{
    mat4 u_camera;
};

struct object_t
{
    mat4 model;
};

layout(std430, binding = 2, column_major) buffer object_constants
{
    object_t objects[];
};

#if VX_SHADER == VX_VERTEX_SHADER

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;

out vec2 v_uv;

void main()
{
    uint iid = VX_INSTANCE_ID;
    gl_Position = u_camera * objects[iid].model * vec4(a_position, 1.0);
    v_uv = a_uv;
}

#elif VX_SHADER == VX_FRAGMENT_SHADER

in vec2 v_uv;

layout(location = 0) uniform sampler2D u_texture;

out vec4 frag_color;

void main()
{
    frag_color = texture(u_texture, v_uv);
}

#endif
