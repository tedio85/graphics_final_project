#version 410

uniform mat4 mvp;
layout (location = 0) in vec3 position;

void main()
{
    gl_Position = mvp * position;
}
