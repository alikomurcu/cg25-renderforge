#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 tex_coord;
layout(location = 3) in vec3 in_tangent;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 frag_tex_coord;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) out mat3 TBN;

// Push constants - MUST match triangle.frag exactly!
layout(push_constant) uniform Push {
    mat4 transform; // transform is actually projection * view * model
    mat4 modelMatrix;
    mat4 normalMat;
    vec4 pointLightPosition;
    vec4 pointLightColor; // w component is intensity
    vec2 screenSize;      // Actual screen size for SSAO UV calculation
    int texture_idx;
    int flags;
    int debugMode;        // 0=normal, 1=SSAO only, 2=normals, 3=depth
}
push;

void main() {
  gl_Position = push.transform * vec4(position, 1.0);
  fragNormal = normalize(mat3(push.normalMat) * normal);
  frag_tex_coord = tex_coord;
  fragWorldPos = (push.modelMatrix * vec4(position, 1.0)).xyz;

  vec3 t = normalize((push.modelMatrix * vec4(in_tangent, 1.0)).xzy);
  vec3 b = cross(fragNormal, t);
  TBN = mat3(t, b, fragNormal);
}