#version 410

layout(location = 0) in vec3 iv3vertex;
layout(location = 1) in vec2 iv2tex_coord;
layout(location = 2) in vec3 iv3normal;

uniform mat4 um4mv;
uniform mat4 um4p;

uniform mat4 shadow_matrix;

uniform vec3 light_pos; // light position in world space

out VertexData
{
    vec3 N; // view space normal
    vec3 L; // view space light vector
    vec3 V;
    vec2 texcoord;
    vec4 shadow_coord;
} vertexData;

void main()
{

    // Calculate view-space coordinate
    vec4 P = um4mv * vec4(iv3vertex, 1.0);

    // Calculate normal in view-space
    // Using only the rotational components of mv_matrix
    vertexData.N = mat3(um4mv) * iv3normal;


    // Calculate light vector
    vertexData.L = mat3(um4mv) * light_pos - P.xyz;

    // Calculate view vector
    vertexData.V = -P.xyz;

    // Calculate the clip-space position of each vertex
    gl_Position = um4p * P;


    vertexData.texcoord = iv2tex_coord;


    // light-space coordinates
    vertexData.shadow_coord = shadow_matrix * vec4(iv3vertex, 1.0);
}
