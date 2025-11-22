#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 tex_coord;

layout (location = 0) out vec3 fragNormal;
layout (location = 1) out vec2 frag_tex_coord;

layout(push_constant) uniform Push {
    mat4 transform;
    mat4 normalMat;
} push;

void main() {
    gl_Position = push.transform * vec4(position, 1.0);
    // Transform normal to world space
    fragNormal = normalize(mat3(push.normalMat) * normal);
    frag_tex_coord = tex_coord;
}