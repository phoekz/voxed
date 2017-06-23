#if VX_SHADER == 0

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 0) uniform mat4 u_camera;
layout(location = 1) uniform mat4 u_model;
out vec3 v_normal;

void main()
{
    gl_Position = u_camera * u_model * vec4(a_position, 1.0);
    v_normal = (u_model * vec4(a_normal, 0.0)).xyz;
}

#elif VX_SHADER == 1

in vec3 v_normal;
layout(location = 2) uniform vec3 u_color;
out vec4 frag_color;

void main()
{
    vec3 n = normalize(v_normal);
    vec3 ld = normalize(vec3(1,1,1));
    float l = 0.5 + 0.5 * max(dot(n, ld), 0.0);
    frag_color = vec4(l * u_color, 1.0);
}

#endif
