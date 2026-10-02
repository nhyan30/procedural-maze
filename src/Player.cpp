#include "Player.hpp"

#include <cmath>

#include "Config.hpp"

namespace maze {
namespace {
constexpr float kPushEpsilon = 0.001f; // keep the circle off the surface
}

void Player::spawnAtCell(const Maze& maze, Maze::Cell cell) {
    const auto [cx, cz] = maze.cellCenter(cell);
    position_ = vec3(cx, 0.0f, cz);
    velocity_ = vec3(0.0f, 0.0f, 0.0f);

    // Face down the first open corridor so the spawn view is interesting.
    camera_.pitch = 0.0f;
    if (!maze.isWall(cell.row - 1, cell.col))      camera_.yaw = 0.0f;             // north (-Z)
    else if (!maze.isWall(cell.row + 1, cell.col)) camera_.yaw = kPi;              // south (+Z)
    else if (!maze.isWall(cell.row, cell.col + 1)) camera_.yaw = 0.5f * kPi;       // east  (+X)
    else                                           camera_.yaw = -0.5f * kPi;      // west  (-X)

    camera_.position = vec3(position_.x, cfg::kEyeHeight, position_.z);
}

void Player::update(const Maze& maze, const InputState& input, float dt) {
    // --- Mouse look ----------------------------------------------------------
    camera_.addLook(input.mouseDX * cfg::kMouseSensitivity,
                    -input.mouseDY * cfg::kMouseSensitivity);

    // --- Desired horizontal velocity ------------------------------------------
    const vec3 fwd = camera_.forwardFlat();
    const vec3 right = camera_.right();
    vec3 wish(0.0f, 0.0f, 0.0f);
    if (input.key(GLFW_KEY_W)) wish += fwd;
    if (input.key(GLFW_KEY_S)) wish -= fwd;
    if (input.key(GLFW_KEY_D)) wish += right;
    if (input.key(GLFW_KEY_A)) wish -= right;

    const float speed = input.key(GLFW_KEY_LEFT_SHIFT) ? cfg::kSprintSpeed
                                                       : cfg::kWalkSpeed;
    if (lengthSq(wish) > 1e-8f) wish = normalize(wish) * speed;

    // Exponential blend towards the target velocity: frame-rate independent
    // acceleration with a natural stop.
    const float blend = 1.0f - std::exp(-cfg::kMoveAccel * dt);
    velocity_ += (wish - velocity_) * blend;

    // --- Integrate with fixed substeps so collisions cannot tunnel ----------
    float remaining = dt;
    while (remaining > 1e-5f) {
        const float h = std::min(remaining, cfg::kMaxPhysicsStep);
        moveStep(maze, velocity_.x * h, velocity_.z * h);
        remaining -= h;
    }

    camera_.position = vec3(position_.x, cfg::kEyeHeight, position_.z);
}

void Player::moveStep(const Maze& maze, float dx, float dz) {
    if (dx != 0.0f) {
        position_.x += dx;
        resolveAxis(maze, 0, dx);
    }
    if (dz != 0.0f) {
        position_.z += dz;
        resolveAxis(maze, 1, dz);
    }
    clampToBounds(maze);
}

// axis 0 = X (columns), axis 1 = Z (rows).
void Player::resolveAxis(const Maze& maze, int axis, float moveDir) {
    const float r = cfg::kPlayerRadius;
    const float s = cfg::kCellSize;
    const float minX = maze.worldMinX();
    const float minZ = maze.worldMinZ();

    const int col0 = static_cast<int>(std::floor((position_.x - r - minX) / s));
    const int col1 = static_cast<int>(std::floor((position_.x + r - minX) / s));
    const int row0 = static_cast<int>(std::floor((position_.z - r - minZ) / s));
    const int row1 = static_cast<int>(std::floor((position_.z + r - minZ) / s));

    for (int row = row0; row <= row1; ++row) {
        for (int col = col0; col <= col1; ++col) {
            if (!maze.isWall(row, col)) continue;

            // Solid cell rectangle.
            const float rx0 = minX + static_cast<float>(col) * s;
            const float rz0 = minZ + static_cast<float>(row) * s;
            const float rx1 = rx0 + s;
            const float rz1 = rz0 + s;

            // Circle vs rectangle: compare against the closest point.
            const float cx = clampf(position_.x, rx0, rx1);
            const float cz = clampf(position_.z, rz0, rz1);
            const float dxp = position_.x - cx;
            const float dzp = position_.z - cz;
            if (dxp * dxp + dzp * dzp >= r * r) continue;

            if (axis == 0) {
                position_.x = (moveDir > 0.0f) ? rx0 - r - kPushEpsilon
                                               : rx1 + r + kPushEpsilon;
            } else {
                position_.z = (moveDir > 0.0f) ? rz0 - r - kPushEpsilon
                                               : rz1 + r + kPushEpsilon;
            }
        }
    }
}

// Hard safety net: the border cells are already walls, but clamping makes it
// impossible to end up outside the maze even in pathological cases.
void Player::clampToBounds(const Maze& maze) {
    const float r = cfg::kPlayerRadius;
    const float lo = maze.worldMinX() + r;
    const float hi = maze.worldMinX() + maze.worldSpan() - r;
    position_.x = clampf(position_.x, lo, hi);
    position_.z = clampf(position_.z, lo, hi);
}

} // namespace maze
