#version 450

layout(set = 0, binding = 0) uniform CameraData {
    mat4 viewProjection;
    mat4 model;
} cameraData;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 0) out vec3 vertexColor;

void main() {
    gl_Position =
        cameraData.viewProjection * cameraData.model *
        vec4(inPosition, 0.0, 1.0);
    vertexColor = inColor;
}
