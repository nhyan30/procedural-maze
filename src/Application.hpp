// Application.hpp - top-level ownership of window, maze, player and renderer,
// plus the frame loop and the exit "win" sequence state machine.
#pragma once

#include <chrono>
#include <memory>
#include <random>
#include <string>

#include "Config.hpp"
#include "Maze.hpp"
#include "Player.hpp"
#include "Renderer.hpp"
#include "Window.hpp"

namespace maze {

struct Options {
    int cellsPerSide = cfg::kDefaultCellsPerSide;
    std::uint64_t seed = 0;
    bool seedExplicit = false;

    // Debug: render N frames, save a BMP and exit (--shot/--frames).
    bool shotEnabled = false;
    std::string shotPath = "maze_screenshot.bmp";
    int shotFrames = 30;

    // Debug: spawn the player on the exit cell (exercises the win sequence).
    bool spawnAtExit = false;

    // Start with the minimap visible (otherwise toggled with M).
    bool minimapOn = false;
};

class Application {
public:
    explicit Application(const Options& options);

    void run();

private:
    std::uint64_t nextSeed();
    void rebuildMaze(int cellsPerSide, std::uint64_t seed, bool fadeIn);
    void handleInput();
    void updateExitSequence(float dt);
    void renderFrame(double time);

    static std::string resolveShaderDir();

    Options options_;
    Window window_;
    std::unique_ptr<Maze> maze_;
    Player player_;
    Renderer renderer_;
    std::mt19937_64 seedRng_;

    enum class Phase { Exploring, FadeOut, FadeIn };
    Phase phase_ = Phase::Exploring;
    float whiteFade_ = 0.0f;
    bool minimapOn_ = false;
    int frameIndex_ = 0;
};

} // namespace maze
