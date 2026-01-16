#version 450

layout(location = 0) in vec4 inPosition;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform Push {
    mat4 transform;
    mat4 normalMat;
    vec4 pointLightPosition;
    vec4 pointLightColor;  // w component is intensity
    uint textureIndex;
} push;

void main() {

    gl_PointSize = 5.0;
    vec4 pos = push.transform * inPosition;
    pos.w = 1;
    gl_Position = pos; 
    fragColor = vec3(0.8, 0.8, 0.8);
}