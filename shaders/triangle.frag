#version 450

layout(set = 0, binding = 0) uniform sampler tex_sampler;
layout(set = 0, binding = 1) uniform texture2D textures[255];
layout(set = 0, binding = 2) uniform sampler2D ssaoTexture;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 frag_tex_coord;
layout(location = 2) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

// Push constants - MUST match triangle.vert exactly!
layout(push_constant) uniform Push {
  mat4 transform; // transform is actually projection * view * model
  mat4 modelMatrix;
  mat4 normalMat;
  vec4 pointLightPosition;
  vec4 pointLightColor; // w component is intensity
  vec2 screenSize;      // Actual screen size for SSAO UV calculation
  int texture_idx;
  int debugMode;        // 0=normal, 1=SSAO only, 2=normals, 3=depth
}
push;

const float AMBIENT_LIGHT = 0.15;

// Calculate point light contribution
vec3 calculatePointLight(vec3 lightPos, vec3 lightColor, float intensity,
                         vec3 normal, vec3 worldPos) {
  vec3 lightDir = lightPos - worldPos;
  float distance = length(lightDir);
  lightDir = normalize(lightDir);
  float attenuation =
      1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
  float diffuse = max(dot(normal, lightDir), 0.0);
  return lightColor * intensity * diffuse * attenuation;
}

void main() {
  // Calculate screen UV for SSAO sampling
  // Both G-buffer and swap chain use Vulkan's coordinate system (Y=0 at top)
  // so no flip is needed
  vec2 screenUV = gl_FragCoord.xy / push.screenSize;
  
  // Sample SSAO
  float ao = texture(ssaoTexture, screenUV).r;
  ao = clamp(ao, 0.0, 1.0);
  if (ao < 0.001) {
    ao = 1.0;
  }

  // Debug modes
  if (push.debugMode == 1) {
    // Mode 1: Show raw SSAO (white = no occlusion, black = full occlusion)
    outColor = vec4(vec3(ao), 1.0);
    return;
  }
  else if (push.debugMode == 2) {
    // Mode 2: Show normals as colors (remap from [-1,1] to [0,1])
    vec3 normalColor = fragNormal * 0.5 + 0.5;
    outColor = vec4(normalColor, 1.0);
    return;
  }
  else if (push.debugMode == 3) {
    // Mode 3: Show depth visualization
    float depth = gl_FragCoord.z;
    outColor = vec4(vec3(depth), 1.0);
    return;
  }

  // Mode 0: Normal rendering with SSAO
  // Sample texture
  vec3 texColor = vec3(1.0, 1.0, 1.0);
  if (push.texture_idx >= 0) {
    texColor = texture(sampler2D(textures[push.texture_idx], tex_sampler),
                       frag_tex_coord).rgb;
  }

  // Calculate point light contribution
  vec3 pointLightContrib =
      calculatePointLight(push.pointLightPosition.xyz, push.pointLightColor.xyz,
                          push.pointLightColor.w, fragNormal, fragWorldPos);

  // Combine ambient (modulated by SSAO) and point light
  vec3 ambient = vec3(AMBIENT_LIGHT) * ao;
  vec3 lighting = ambient + pointLightContrib;

  // Apply lighting to texture
  vec3 finalColor = texColor * lighting;
  outColor = vec4(finalColor, 1.0);
}