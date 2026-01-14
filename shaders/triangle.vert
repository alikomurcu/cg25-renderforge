#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 tex_coord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 frag_tex_coord;
layout(location = 2) out vec3 fragWorldPos;

// Push constants - MUST match triangle.frag exactly!
layout(push_constant) uniform Push {
  mat4 transform;
  mat4 normalMat;
  vec4 pointLightPosition;
  vec4 pointLightColor; // w component is intensity
  vec2 screenSize;      // For SSAO UV calculation
  int texture_idx;
  int debugMode;        // 0=normal, 1=SSAO only, 2=normals, 3=depth
}
push;

void main() {
  gl_Position = push.transform * vec4(position, 1.0);
  fragNormal = normalize(mat3(push.normalMat) * normal);
  frag_tex_coord = tex_coord;
  fragWorldPos = (push.transform * vec4(position, 1.0)).xyz;
}