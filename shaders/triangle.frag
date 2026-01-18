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
    vec4 pointLightPosition;
    vec4 pointLightColor;  // w component is intensity
    int texture_idx;
    int flags;
} push;

const float AMBIENT_LIGHT = 0.02;

// Calculate point light contribution
vec3 calculatePointLight(vec3 lightPos, vec3 lightColor, float intensity, 
                         vec3 normal, vec3 worldPos) {
    // Calculate direction from fragment to point light
    vec3 lightDir = lightPos - worldPos;
    float distance = length(lightDir);
    lightDir = normalize(lightDir);
    
    // Calculate attenuation with distance (quadratic falloff)
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
    
    // Diffuse shading calculation for point light
    float diffuse = max(dot(normal, lightDir), 0.0);
    
    return lightColor * intensity * diffuse * attenuation;
}

void main() {
    // Sample texture
    vec3 texColor = vec3(1.0, 1.0, 1.0);
    vec3 normal = fragNormal;
    int num_textures = int(push.flags / 10) % 10;
    bool has_normal = (push.flags % 10) > 0;
    if(num_textures > 0){
        //texColor = vec3(1.0, 0.0, 0.0);
        texColor = texture(sampler2D(textures[push.texture_idx], tex_sampler), frag_tex_coord).rgb;
    }
    if(has_normal){
        vec3 normal_tex = texture(sampler2D(textures[push.texture_idx + 1], tex_sampler), frag_tex_coord).rgb;
        normal = normalize(normal_tex * 2.0 - 1.0);
    }


    // Calculate point light contribution
    vec3 pointLightContrib = calculatePointLight(
        push.pointLightPosition.xyz,
        push.pointLightColor.xyz,
        push.pointLightColor.w,  // intensity from w component
        normal,
        fragWorldPos
    );
    
    // Combine ambient and point light
    vec3 lighting = vec3(AMBIENT_LIGHT) + pointLightContrib;

    // Apply lighting to texture
    vec3 finalColor = texColor * lighting;
    outColor = vec4(finalColor, 1.0);
    if(has_normal)
        outColor = vec4(normal, 1.0);
}