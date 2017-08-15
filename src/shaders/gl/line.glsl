layout(std430, binding = 0) restrict readonly buffer vertex_t
{
    float vertex_data[];
};

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

out vec4 v_color;

void main()
{
    vec3 pos;
    pos.x = vertex_data[gl_VertexID * 3 + 0];
    pos.y = vertex_data[gl_VertexID * 3 + 1];
    pos.z = vertex_data[gl_VertexID * 3 + 2];

    uint iid = VX_INSTANCE_ID;
    gl_Position = global.camera * objects[iid].model * vec4(pos, 1.0);
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
