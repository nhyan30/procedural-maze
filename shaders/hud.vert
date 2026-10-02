#version 330 core
// HUD overlay: pixel-space 2D geometry, origin at the top-left of the
// viewport, Y growing downwards. uResolution converts pixels to NDC.
layout (location = 0) in vec2 aPos;   // pixel coords
layout (location = 1) in vec2 aUV;    // glyph atlas coords
layout (location = 2) in vec4 aColor; // per-vertex tint + alpha

uniform vec2 uResolution;             // viewport size in pixels

out vec2 vUV;
out vec4 vColor;

void main() {
    vec2 ndc = vec2(aPos.x / uResolution.x * 2.0 - 1.0,
                    1.0 - aPos.y / uResolution.y * 2.0);
    gl_Position = vec4(ndc, 0.0, 1.0);
    vUV = aUV;
    vColor = aColor;
}
