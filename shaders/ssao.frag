#version 450

// SSAO calculation shader
// Based on LearnOpenGL SSAO implementation

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out float fragOcclusion;

// G-buffer textures
layout(set = 0, binding = 0) uniform sampler2D gPosition;
layout(set = 0, binding = 1) uniform sampler2D gNormal;

// Noise texture (4x4 tiled)
layout(set = 0, binding = 2) uniform sampler2D texNoise;

// Sample kernel (64 samples)
layout(set = 0, binding = 3) uniform KernelUBO {
    vec4 samples[64];
} kernel;

// SSAO parameters
layout(push_constant) uniform SSAOParams {
    mat4 projection;
    vec2 noiseScale;  // screen dimensions / 4
    float radius;
    float bias;
    int kernelSize;
} params;

void main() {
    // Get input from G-buffer
    vec3 fragPos = texture(gPosition, fragTexCoord).xyz;
    vec3 normal = normalize(texture(gNormal, fragTexCoord).xyz);
    
    // Get random rotation vector from noise texture
    vec3 randomVec = normalize(texture(texNoise, fragTexCoord * params.noiseScale).xyz);
    
    // Create TBN change-of-basis matrix: from tangent-space to view-space
    // Gram-Schmidt process to orthonormalize
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    
    // Iterate over sample kernel and calculate occlusion
    float occlusion = 0.0;
    
    for (int i = 0; i < params.kernelSize; ++i) {
        // Get sample position (in tangent space)
        vec3 sampleTangent = kernel.samples[i].xyz;
        
        // Transform sample to view space
        vec3 samplePos = TBN * sampleTangent;
        samplePos = fragPos + samplePos * params.radius;
        
        // Project sample position to screen space
        vec4 offset = params.projection * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;  // Perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5;  // Transform to [0, 1] range
        
        // Get depth at sample position from G-buffer
        float sampleDepth = texture(gPosition, offset.xy).z;
        
        // Range check: avoid occlusion from far-away surfaces
        // The idea: if the sampled depth is much further than our radius,
        // it shouldn't contribute to occlusion
        float rangeCheck = smoothstep(0.0, 1.0, params.radius / abs(fragPos.z - sampleDepth));
        
        // Check if sample is occluded
        // If the actual geometry is closer than our sample point, it's occluded
        occlusion += (sampleDepth <= samplePos.z - params.bias ? 1.0 : 0.0) * rangeCheck;
    }
    
    // Average and invert (1.0 = no occlusion, 0.0 = fully occluded)
    occlusion = 1.0 - (occlusion / float(params.kernelSize));
    
    fragOcclusion = occlusion;
}
