#version 450

// Inputs from vertex shader
layout(location = 0) in vec3 fragViewPos;
layout(location = 1) in vec3 fragViewNormal;
layout(location = 2) in vec2 fragTexCoord;

// Multiple Render Targets (MRT)
layout(location = 0) out vec4 gPosition;  // View-space position
layout(location = 1) out vec4 gNormal;    // View-space normal

void main() {
    // Store view-space position
    // Using w=1.0 to indicate valid fragment (for edge detection)
    gPosition = vec4(fragViewPos, 1.0);
    
    // Store view-space normal (normalized)
    // Pack into [0,1] range is not needed since we use RGBA16F
    gNormal = vec4(normalize(fragViewNormal), 1.0);
}
