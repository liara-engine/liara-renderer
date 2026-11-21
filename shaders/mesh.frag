#version 450

// The fragment shader of every mesh the renderer draws: the interpolated vertex colour, opaque.

layout(location = 0) in vec3 inColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(inColor, 1.0);
}
