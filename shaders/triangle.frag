#version 450

layout(set = 0, binding = 0) uniform sampler tex_sampler;
layout(set = 0, binding = 1) uniform texture2D textures[256];

layout (location = 0) in vec3 fragNormal;
layout (location = 1) in vec2 frag_tex_coord;
layout (location = 2) in vec3 fragWorldPos;

layout (location=0) out vec4 outColor;

layout(push_constant) uniform Push {
    mat4 transform;
    mat4 normalMat;
    float totalTime;
    float radius;
    float height;
} push;

const vec3 LIGHT_COLOR = vec3(1.0, 1.0, 1.0);
const float AMBIENT_LIGHT = 0.02;
const float ORBIT_SPEED = 90.0; // degrees per second

void main() {
    // Calculate orbiting point light position in shader
    float angle = push.totalTime * radians(ORBIT_SPEED);
    vec3 pointLightPos = vec3(
        push.radius * cos(angle),
        push.height,
        push.radius * sin(angle)
    );
    
    // Calculate direction from fragment to point light
    vec3 lightDir = pointLightPos - fragWorldPos;
    float distance = length(lightDir);
    lightDir = normalize(lightDir);
    
    // Calculate attenuation with distance
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
    
    // Diffuse shading calculation for point light
    float diffuse = max(dot(fragNormal, lightDir), 0.0);
    float lightIntensity = AMBIENT_LIGHT + (diffuse * attenuation);
    
    // Sample texture
    vec3 texColor = texture(sampler2D(textures[0], tex_sampler), frag_tex_coord).rgb;
    
    // Apply diffuse lighting to texture
    vec3 finalColor = texColor * lightIntensity * LIGHT_COLOR;
    outColor = vec4(finalColor, 1.0);
}