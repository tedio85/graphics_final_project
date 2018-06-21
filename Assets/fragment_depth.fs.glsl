#version 410

out vec4 fragColor;
uniform sampler2D tex;


void main()
{
    //fragColor = vec4(vec3(gl_FragCoord.z), 1.0);
    fragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}
