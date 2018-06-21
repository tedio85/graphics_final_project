#version 410 core

uniform mat4 mvp;
layout(location = 0) in vec3 iv3vertex;
layout(location = 1) in vec2 iv2tex_coord;
layout(location = 2) in vec3 iv3normal;

void main()
{
	gl_Position = mvp * vec4(iv3vertex, 1.0f);
}
