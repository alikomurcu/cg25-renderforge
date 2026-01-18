#version 450

// Vertex inputs
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 tex_coord;
layout(location = 3) in vec3 in_tangent;

// Outputs to fragment shader
layout(location = 0) out vec3 fragViewPos;
layout(location = 1) out vec3 fragViewNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out mat3 TBN;

// Push constants
layout(push_constant) uniform Push {
    mat4 modelView;     // Model * View matrix
    mat4 projection;    // Projection matrix
    mat4 normalMat;     // Normal matrix (transpose(inverse(modelView)))
} push;

void main() {
    // Transform position to view space
    vec4 viewPos = push.modelView * vec4(position, 1.0);
    fragViewPos = viewPos.xyz;
    
    // Transform normal to view space
    fragViewNormal = normalize(mat3(push.normalMat) * normal);
    
    // Pass through texture coordinates
    fragTexCoord = tex_coord;
    
    //https://learnopengl.com/Advanced-Lighting/Normal-Mapping
    vec3 t = normalize((push.modelView * vec4(in_tangent, 1.0)).xzy);
    vec3 b = cross(fragViewNormal, t);
    TBN = mat3(t, b, fragViewNormal);
    // Final clip position
    gl_Position = push.projection * viewPos;
}
