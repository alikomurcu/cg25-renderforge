#version 450

layout(set = 0, binding = 0) uniform sampler tex_sampler;
layout(set = 0, binding = 1) uniform texture2D textures[256];

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec2 frag_tex_coord;

layout (location=0) out vec4 outColor;


layout(push_constant) uniform Push {
    mat4 transform;
    vec3 color;
} push;

void main() {
    outColor = texture(sampler2D(textures[0], tex_sampler), frag_tex_coord);
}