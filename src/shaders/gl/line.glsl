#if VX_SHADER == 0

layout(location = 0) in vec3 a_position;

layout(location = 0) uniform mat4 u_camera;
layout(location = 1) uniform mat4 u_model;

void main()
{
    gl_Position = u_camera * u_model * vec4(a_position, 1.0);
}

#elif VX_SHADER == 1

layout(location = 2) uniform vec3 u_color;

out vec4 frag_color;

void main()
{
    frag_color = vec4(u_color, 1.0);
}

#endif
