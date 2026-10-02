#version 330 core
// HUD fragment pass: ink comes from the single-channel glyph atlas; uSolid
// bypasses the atlas for plain filled rectangles (the text panel backdrop).
in vec2 vUV;
in vec4 vColor;

out vec4 fragColor;

uniform sampler2D uFont;  // GL_R8 atlas, ink in the red channel
uniform float uSolid;     // 1.0 = solid rect, 0.0 = glyph ink

void main() {
    float ink = mix(texture(uFont, vUV).r, 1.0, uSolid);
    float alpha = vColor.a * ink;
    if (alpha <= 0.003) discard;
    fragColor = vec4(vColor.rgb, alpha);
}
