// Math.hpp - minimal linear algebra used by the game (header-only).
//
// The project deliberately avoids GLM: the subset of math an FPS camera and
// a grid renderer need is small, and writing it out keeps the dependency
// list at exactly one library (GLFW). Matrices are column-major, matching
// OpenGL's expectation for glUniformMatrix4fv.
#pragma once

#include <cmath>

namespace maze {

constexpr float kPi = 3.14159265358979323846f;

inline float radians(float degrees) { return degrees * (kPi / 180.0f); }

// ---------------------------------------------------------------------------
// vec2 / vec3 / vec4
// ---------------------------------------------------------------------------
struct vec2 {
    float x = 0.0f, y = 0.0f;

    constexpr vec2() = default;
    constexpr vec2(float xx, float yy) : x(xx), y(yy) {}
};

struct vec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;

    constexpr vec3() = default;
    constexpr vec3(float xx, float yy, float zz) : x(xx), y(yy), z(zz) {}
    explicit constexpr vec3(float s) : x(s), y(s), z(s) {}

    constexpr vec3& operator+=(const vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    constexpr vec3& operator-=(const vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    constexpr vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
    constexpr vec3& operator/=(float s) { const float inv = 1.0f / s; x *= inv; y *= inv; z *= inv; return *this; }
};

inline constexpr vec3 operator+(const vec3& a, const vec3& b) { return vec3(a.x + b.x, a.y + b.y, a.z + b.z); }
inline constexpr vec3 operator-(const vec3& a, const vec3& b) { return vec3(a.x - b.x, a.y - b.y, a.z - b.z); }
inline constexpr vec3 operator-(const vec3& a) { return vec3(-a.x, -a.y, -a.z); }
inline constexpr vec3 operator*(const vec3& a, float s) { return vec3(a.x * s, a.y * s, a.z * s); }
inline constexpr vec3 operator*(float s, const vec3& a) { return a * s; }
inline constexpr vec3 operator/(const vec3& a, float s) { return vec3(a.x / s, a.y / s, a.z / s); }

inline constexpr float dot(const vec3& a, const vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline constexpr vec3 cross(const vec3& a, const vec3& b) {
    return vec3(a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x);
}
inline float length(const vec3& a) { return std::sqrt(dot(a, a)); }
inline float lengthSq(const vec3& a) { return dot(a, a); }
inline vec3 normalize(const vec3& a) {
    const float len = length(a);
    return (len > 1e-8f) ? a / len : vec3(0.0f, 0.0f, 0.0f);
}
inline vec3 lerp(const vec3& a, const vec3& b, float t) { return a + (b - a) * t; }
inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// ---------------------------------------------------------------------------
// mat4 - column-major, i.e. m[col * 4 + row]
// ---------------------------------------------------------------------------
struct mat4 {
    float m[16];

    static mat4 identity() {
        mat4 r{};
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
        return r;
    }

    static mat4 translate(const vec3& t) {
        mat4 r = identity();
        r.m[12] = t.x; r.m[13] = t.y; r.m[14] = t.z;
        return r;
    }

    static mat4 scale(const vec3& s) {
        mat4 r{};
        r.m[0] = s.x; r.m[5] = s.y; r.m[10] = s.z; r.m[15] = 1.0f;
        return r;
    }

    // Rotate about the +Y axis (counter-clockwise when viewed from above).
    static mat4 rotateY(float rad) {
        mat4 r = identity();
        const float c = std::cos(rad), s = std::sin(rad);
        r.m[0] = c;  r.m[2] = -s;
        r.m[8] = s;  r.m[10] = c;
        return r;
    }

    // Standard OpenGL perspective projection (right-handed, -Z forward).
    static mat4 perspective(float fovYRad, float aspect, float zNear, float zFar) {
        const float f = 1.0f / std::tan(fovYRad * 0.5f);
        mat4 r{};
        r.m[0]  = f / aspect;
        r.m[5]  = f;
        r.m[10] = (zFar + zNear) / (zNear - zFar);
        r.m[11] = -1.0f;
        r.m[14] = (2.0f * zFar * zNear) / (zNear - zFar);
        return r;
    }

    // Orthographic projection. Reversed top/bottom is legal and used by the
    // minimap to flip world Z onto screen Y.
    static mat4 ortho(float left, float right, float bottom, float top,
                      float zNear, float zFar) {
        mat4 r = identity();
        r.m[0]  = 2.0f / (right - left);
        r.m[5]  = 2.0f / (top - bottom);
        r.m[10] = -2.0f / (zFar - zNear);
        r.m[12] = -(right + left) / (right - left);
        r.m[13] = -(top + bottom) / (top - bottom);
        r.m[14] = -(zFar + zNear) / (zFar - zNear);
        return r;
    }

    static mat4 lookAt(const vec3& eye, const vec3& center, const vec3& up) {
        const vec3 zAxis = normalize(eye - center);   // camera backward
        const vec3 xAxis = normalize(cross(up, zAxis));
        const vec3 yAxis = cross(zAxis, xAxis);
        mat4 r = identity();
        r.m[0] = xAxis.x; r.m[4] = xAxis.y; r.m[8]  = xAxis.z;
        r.m[1] = yAxis.x; r.m[5] = yAxis.y; r.m[9]  = yAxis.z;
        r.m[2] = zAxis.x; r.m[6] = zAxis.y; r.m[10] = zAxis.z;
        r.m[12] = -dot(xAxis, eye);
        r.m[13] = -dot(yAxis, eye);
        r.m[14] = -dot(zAxis, eye);
        return r;
    }
};

// (a * b) applies b first, then a - the usual GL convention.
inline mat4 operator*(const mat4& a, const mat4& b) {
    mat4 r{};
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k)
                sum += a.m[k * 4 + row] * b.m[col * 4 + k];
            r.m[col * 4 + row] = sum;
        }
    }
    return r;
}

} // namespace maze
