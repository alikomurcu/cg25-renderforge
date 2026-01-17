#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in ivec4 particleFlags;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform Push {
    mat4 transform;
    mat4 normalMat;
    vec4 pointLightPosition;
    vec4 pointLightColor;  // w component is intensity
    uint textureIndex;
} push;

vec3 colors[] = vec3[](
    vec3(0.1 ,0.1, 0.1),
    vec3(0.2 ,0.2, 0.2),
    vec3(0.3 ,0.3, 0.3),
    vec3(0.4 ,0.4, 0.4),
    vec3(0.5 ,0.5, 0.5),
    vec3(0.6 ,0.6, 0.6),
    vec3(0.7 ,0.7, 0.7),
    vec3(0.8 ,0.8, 0.8),
    vec3(0.9 ,0.9, 0.9),
    vec3(1.0 ,1.0, 1.0)
);


void main() {
    uint colorsSize = 10;
    int ttl = particleFlags.x;
    int max_ttl = particleFlags.w;
    gl_PointSize = 6;
    gl_Position = push.transform * inPosition; 
    int percentage =  ((max_ttl-ttl) * 100) / max_ttl;
    int index = max(0, int(round(percentage * float(colorsSize) / 100.0)) - 1);
    if(particleFlags.y == 1){
        fragColor = vec3(1.0, 0.0, 0.0);
    }else {
        fragColor = colors[index];
    }
}