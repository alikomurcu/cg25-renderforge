#version 450

// SSAO blur shader
// Simple 4x4 box blur to remove noise pattern from SSAO

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out float fragBlurred;

layout(set = 0, binding = 0) uniform sampler2D ssaoInput;

void main() {
    // Get texel size
    vec2 texelSize = 1.0 / vec2(textureSize(ssaoInput, 0));
    
    // 4x4 box blur (matches noise texture size)
    float result = 0.0;
    for (int x = -2; x < 2; ++x) {
        for (int y = -2; y < 2; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(ssaoInput, fragTexCoord + offset).r;
        }
    }
    
    fragBlurred = result / 16.0;  // Average of 16 samples (4x4)
}
