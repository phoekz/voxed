layout(std430, binding = 0) restrict readonly buffer vertex_t
{
    float vertex_data[];
};

layout(std430, binding = 1, column_major) buffer global_constants
{
    mat4 u_projection;
};

#if VX_SHADER == VX_VERTEX_SHADER

out vec2 v_frag_uv;
out vec4 v_frag_color;

void main()
{
    vec2 pos;
    pos.x = vertex_data[gl_VertexID * 5 + 0];
    pos.y = vertex_data[gl_VertexID * 5 + 1];

    vec2 uv;
    uv.x = vertex_data[gl_VertexID * 5 + 2];
    uv.y = vertex_data[gl_VertexID * 5 + 3];

    vec4 color;
    color = unpackUnorm4x8(floatBitsToUint(vertex_data[gl_VertexID * 5 + 4]));

    v_frag_uv = uv;
    v_frag_color = color;
    gl_Position = u_projection * vec4(pos, 0.0, 1.0);
}

#elif VX_SHADER == VX_FRAGMENT_SHADER

layout(location = 0) uniform sampler2D u_texture;

in vec2 v_frag_uv;
in vec4 v_frag_color;

out vec4 frag_color;

void main()
{
    frag_color = v_frag_color * texture(u_texture, v_frag_uv.st);
}

#endif
