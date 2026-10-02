#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;

uniform sampler2D uTex;
uniform vec3 uCamPos;

uniform vec3 uAmbient;
uniform vec3 uDirDir;               // direction the directional light travels
uniform vec3 uDirColor;

uniform vec3  uPointPos[2];         // [0] = player torch, [1] = exit beacon
uniform vec3  uPointColor[2];
uniform float uPointIntensity[2];
uniform vec3  uPointAtten[2];       // (constant, linear, quadratic)

uniform vec3  uFogColor;
uniform float uFogDensity;

uniform float uSpecular;
uniform float uShininess;

out vec4 FragColor;

void main() {
    vec3 albedo = texture(uTex, vUV).rgb;
    vec3 n = normalize(vNormal);
    vec3 v = normalize(uCamPos - vWorldPos);

    // Ambient term plus a faint cool directional light so unlit geometry
    // stays readable without competing with the torch.
    vec3 lit = albedo * uAmbient;
    {
        float ndl = max(dot(n, -uDirDir), 0.0);
        vec3 h = normalize(v - uDirDir);
        float spec = pow(max(dot(n, h), 0.0), uShininess);
        lit += albedo * uDirColor * ndl;
        lit += uDirColor * spec * 0.25;
    }

    // Blinn-Phong point lights with quadratic attenuation.
    for (int i = 0; i < 2; ++i) {
        vec3 toLight = uPointPos[i] - vWorldPos;
        float dist = length(toLight);
        vec3 l = toLight / max(dist, 1e-4);

        float att = 1.0 / (uPointAtten[i].x + uPointAtten[i].y * dist +
                           uPointAtten[i].z * dist * dist);
        float ndl = max(dot(n, l), 0.0);
        vec3 h = normalize(l + v);
        float spec = pow(max(dot(n, h), 0.0), uShininess);

        vec3 radiance = uPointColor[i] * uPointIntensity[i] * att;
        lit += albedo * radiance * ndl;
        lit += radiance * spec * 0.60;
    }

    // Exponential-squared distance fog towards the void colour.
    float fogDist = length(uCamPos - vWorldPos);
    float fog = exp(-pow(fogDist * uFogDensity, 2.0));
    lit = mix(uFogColor, lit, clamp(fog, 0.0, 1.0));

    // Manual gamma correction.
    lit = pow(lit, vec3(1.0 / 2.2));
    FragColor = vec4(lit, 1.0);
}
