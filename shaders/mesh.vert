#version 450

// The vertex shader of every mesh the renderer draws. Positions are the contract's normalized device coordinates,
// y up; the viewport, not this shader, flips them into Vulkan's y-down framebuffer.

// True when the target encodes its writes to sRGB, so the colours, sRGB-encoded in the packet's convention, are
// decoded to linear first. Decoding here rather than per pixel interpolates in linear space.
layout(constant_id = 0) const bool DECODE_SRGB = true;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in uint inColor;  // 0xAARRGGBB

layout(location = 0) out vec3 outColor;

vec3 SrgbToLinear(vec3 encoded) {
    vec3 low = encoded / 12.92;
    vec3 high = pow((encoded + 0.055) / 1.055, vec3(2.4));
    return mix(high, low, lessThanEqual(encoded, vec3(0.04045)));
}

void main() {
    gl_Position = vec4(inPosition, 1.0);
    vec3 encoded = vec3(float((inColor >> 16u) & 0xFFu), float((inColor >> 8u) & 0xFFu), float(inColor & 0xFFu)) / 255.0;
    outColor = DECODE_SRGB ? SrgbToLinear(encoded) : encoded;
}
