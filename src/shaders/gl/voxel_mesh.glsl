#define RENDER_FLAG_AMBIENT_OCCLUSION (1 << 0)
#define RENDER_FLAG_DIRECTIONAL_LIGHT (1 << 1)

layout(std430, binding = 0) restrict readonly buffer vertex_t
{
    float vertex_data[];
};

layout(std430, binding = 1, column_major) buffer global_constants
{
    mat4 world_to_clip;
    uint flags;
} global;

#if VX_SHADER == VX_VERTEX_SHADER

out vec3 v_normal;
out vec4 v_color;
out float v_ao;

void main()
{
    vec3 pos;
    pos.x = vertex_data[gl_VertexID * 8 + 0];
    pos.y = vertex_data[gl_VertexID * 8 + 1];
    pos.z = vertex_data[gl_VertexID * 8 + 2];

    vec4 color;
    color = unpackUnorm4x8(floatBitsToUint(vertex_data[gl_VertexID * 8 + 3]));

    vec3 normal;
    normal.x = vertex_data[gl_VertexID * 8 + 4];
    normal.y = vertex_data[gl_VertexID * 8 + 5];
    normal.z = vertex_data[gl_VertexID * 8 + 6];

    float ao;
    ao = vertex_data[gl_VertexID * 8 + 7];

    v_normal = normal;
    v_color = color;
    v_ao = ao;
    v_ao = bool(global.flags & RENDER_FLAG_AMBIENT_OCCLUSION) ? 1.0 - ao : 1.0;
    gl_Position = global.world_to_clip * vec4(pos, 1.0);
}

#elif VX_SHADER == VX_FRAGMENT_SHADER

in vec3 v_normal;
in vec4 v_color;
in float v_ao;

out vec4 frag_color;

void main()
{
    vec3 n = normalize(v_normal);
    vec3 ld = normalize(vec3(1.0, 3.0, 1.0));
    float ao = smoothstep(0.0, 1.0, v_ao);
    float l = bool(global.flags & RENDER_FLAG_DIRECTIONAL_LIGHT) ? max(dot(n, ld), 0.0) : 1.0;
    float li = 0.25 + 0.75 * ao * l;
    frag_color = vec4(v_color.rgb * li, 1.0);
}

#endif
