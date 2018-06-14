#version 410

layout(location = 0) out vec4 fragColor;

uniform mat4 um4mv;
uniform mat4 um4p;

in VertexData
{
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 V; // eye space halfway vector
    vec2 texcoord;
} vertexData;

// Material properties
uniform vec3 diffuse_albedo = vec3(0.5, 0.2, 0.7);
uniform vec3 specular_albedo = vec3(0.7);
uniform float specular_power = 200.0;


uniform sampler2D tex;

void main()
{
    vec3 texColor = texture(tex,vertexData.texcoord).rgb;
    
    /*
    // Normalize the incoming N, L and V vectors
    vec3 N = normalize(vertexData.N);
    vec3 L = normalize(vertexData.L);
    vec3 V = normalize(vertexData.V);
    vec3 H = normalize(L + V);
    
    // Compute the diffuse and specular components for each fragment
    vec3 diffuse = max(dot(N, L), 0.0) * diffuse_albedo * texColor;
    vec3 specular = pow(max(dot(N, H), 0.0), specular_power) * specular_albedo * texColor;
    
    // Write final color to the framebuffer
    vec3 result = diffuse + specular;
    fragColor = vec4(N, 1.0);
    */
    
    
    fragColor = vec4(texColor, 1.0);
}
