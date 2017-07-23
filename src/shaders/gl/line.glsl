layout(std430, binding = 1, column_major) buffer global_constants
{
    mat4 camera;
} global;

struct object_t
{
    mat4 model;
    vec4 color;
};

layout(std430, binding = 2, column_major) buffer object_constants
{
    object_t objects[];
};

#if VX_SHADER == VX_VERTEX_SHADER

layout(location = 0) in vec3 a_position;

out vec4 v_color;

void main()
{
    uint iid = VX_INSTANCE_ID;
    gl_Position = global.camera * objects[iid].model * vec4(a_position, 1.0);
    v_color = objects[iid].color;
}

#elif VX_SHADER == VX_FRAGMENT_SHADER

in vec4 v_color;

out vec4 frag_color;

void main()
{
    frag_color = v_color;
}

#endif
