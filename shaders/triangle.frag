#version 450

layout(set = 0, binding = 0) uniform sampler tex_sampler;
layout(set = 0, binding = 1) uniform texture2D textures[256];

layout (location = 0) in vec3 fragNormal;
layout (location = 1) in vec2 frag_tex_coord;

layout (location=0) out vec4 outColor;

layout(push_constant) uniform Push {
    mat4 transform;
    mat4 normalMat;
} push;

const vec3 DIR_TO_DIRLIGHT = normalize(vec3(1.0, -3.0, -1.0));
const float AMBIENT_LIGHT = 0.02;
const vec3 LIGHT_COLOR = vec3(1.0, 1.0, 1.0);

void main() {
    // Diffuse shading calculation
    float diffuse = max(dot(fragNormal, DIR_TO_DIRLIGHT), 0.0);
    float lightIntensity = AMBIENT_LIGHT + diffuse;
    
    // Sample texture
    vec3 texColor = texture(sampler2D(textures[0], tex_sampler), frag_tex_coord).rgb;
    
    // Apply diffuse lighting to texture
    vec3 finalColor = texColor * lightIntensity * LIGHT_COLOR;
    outColor = vec4(finalColor, 1.0);
}