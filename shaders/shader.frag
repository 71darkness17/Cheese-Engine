#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Object {
    mat4 model;
    uint textureHandler;
} pc;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(binding = 1) uniform sampler2DArray texSampler[128];

void main() {
    uint arrayId = pc.textureHandler >> 16;
    uint layerId = pc.textureHandler & 0xFFFF;
    if(arrayId == 0 && layerId == 0) {
        outColor = vec4(fragColor, 1.0);
    } else {
        outColor = texture(texSampler[arrayId], vec3(fragTexCoord, layerId));
    }
}