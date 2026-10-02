#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec4 iOffsetUV; // xyz = world offset, w = uv scale

uniform mat4 uViewProj;
uniform vec3 uModelScale;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;

void main() {
    vec3 world = aPos * uModelScale + iOffsetUV.xyz;
    vWorldPos = world;
    vNormal = aNormal;
    vUV = aUV * iOffsetUV.w;
    gl_Position = uViewProj * vec4(world, 1.0);
}
