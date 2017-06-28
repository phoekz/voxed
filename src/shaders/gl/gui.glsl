#if VX_SHADER == 0

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec4 a_color;

layout(binding = 1, column_major) uniform Matrix
{
mat4 u_projection;
};

out vec2 v_frag_uv;
out vec4 v_frag_color;

void main()
{
    v_frag_uv = a_uv;
    v_frag_color = a_color;
    gl_Position = u_projection * vec4(a_position.xy, 0, 1);
}

#elif VX_SHADER == 1

uniform sampler2D u_texture;

in vec2 v_frag_uv;
in vec4 v_frag_color;

out vec4 frag_color;

void main()
{
    frag_color = v_frag_color * texture(u_texture, v_frag_uv.st);
}

#endif
