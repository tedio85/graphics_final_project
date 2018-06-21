#version 410

uniform sampler2D tex;

out vec4 color;

in VS_OUT
{
    vec2 texcoord;
} fs_in;



void main()
{
    float depth = texture(tex, fs_in.texcoord).r;
    color = vec4(vec3(depth), 1.0);
}
