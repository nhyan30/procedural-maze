// Camera.hpp - first-person yaw/pitch camera.
//
// Deliberately quaternion-free: an FPS camera with pitch clamped to +-89
// degrees needs nothing more than two Euler angles, and avoiding a rotation
// representation keeps the hand-rolled math surface small and auditable.
#pragma once

#include "Config.hpp"
#include "Math.hpp"

namespace maze {

class Camera {
public:
    vec3 position;      // eye position in world space
    float yaw = 0.0f;   // radians, 0 looks down -Z, positive turns right
    float pitch = 0.0f; // radians, positive looks up

    // Full 3D view direction (includes pitch).
    vec3 forward() const {
        const float cp = std::cos(pitch);
        return vec3(std::sin(yaw) * cp, std::sin(pitch), -std::cos(yaw) * cp);
    }

    // Ground-plane view direction (never tilts) - used for movement.
    vec3 forwardFlat() const {
        return vec3(std::sin(yaw), 0.0f, -std::cos(yaw));
    }

    vec3 right() const {
        return vec3(std::cos(yaw), 0.0f, std::sin(yaw));
    }

    void addLook(float deltaYaw, float deltaPitch) {
        yaw += deltaYaw;
        pitch = clampf(pitch + deltaPitch, -cfg::kMaxPitch, cfg::kMaxPitch);
        // Keep yaw in [-pi, pi] so the float stays precise over long sessions.
        if (yaw > kPi) yaw -= 2.0f * kPi;
        if (yaw < -kPi) yaw += 2.0f * kPi;
    }

    mat4 view() const {
        return mat4::lookAt(position, position + forward(), vec3(0.0f, 1.0f, 0.0f));
    }

    mat4 projection(float aspect) const {
        return mat4::perspective(radians(cfg::kFovDegrees), aspect,
                                 cfg::kZNear, cfg::kZFar);
    }
};

} // namespace maze
