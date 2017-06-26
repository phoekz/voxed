#if VX_SHADER == 0

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 0) uniform mat4 u_camera;
layout(location = 1) uniform mat4 u_model;
out vec2 v_uv;

void main()
{
    gl_Position = u_camera * u_model * vec4(a_position, 1.0);
    v_uv = a_uv;
}

#elif VX_SHADER == 1

in vec2 v_uv;
layout(location = 2) uniform sampler2D u_texture;
out vec4 frag_color;

void main()
{
    vec4 c = texture(u_texture, v_uv).rgba;
    frag_color = vec4(c.rgba);
}

#endif
