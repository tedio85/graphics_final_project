#version 410

uniform mat4 mvp;
layout (location = 0) in vec3 position;
layout(location = 1) in vec2 iv2tex_coord;
layout(location = 2) in vec3 iv3normal;

void main()
{
    gl_Position = mvp * vec4(position, 1.0);
}
