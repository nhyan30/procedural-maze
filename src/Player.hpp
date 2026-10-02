// Player.hpp - first-person movement + collision against the maze grid.
//
// The player is a vertical cylinder approximated by a circle of radius
// kPlayerRadius on the XZ plane. Collision uses the classic two-pass
// "move and clamp" scheme: move along one axis, push out of any solid cell
// the circle overlaps, then repeat for the other axis. This gives smooth
// wall sliding and is robust against tunnelling because the frame delta is
// split into fixed-size substeps.
#pragma once

#include "Camera.hpp"
#include "Maze.hpp"
#include "Window.hpp"

namespace maze {

class Player {
public:
    const Camera& camera() const { return camera_; }

    // Position of the player's feet on the ground plane (y = 0).
    const vec3& position() const { return position_; }
    const vec3& velocity() const { return velocity_; }

    void spawnAtCell(const Maze& maze, Maze::Cell cell);
    void update(const Maze& maze, const InputState& input, float dt);

private:
    void moveStep(const Maze& maze, float dx, float dz);
    void resolveAxis(const Maze& maze, int axis, float moveDir);
    void clampToBounds(const Maze& maze);

    Camera camera_;
    vec3 position_;
    vec3 velocity_;
};

} // namespace maze
