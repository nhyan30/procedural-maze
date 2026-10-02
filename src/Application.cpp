#include "Application.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <vector>

#include "DebugCapture.hpp"

namespace maze {
namespace {

std::chrono::steady_clock::time_point nowClock() {
    return std::chrono::steady_clock::now();
}

float secondsSince(const std::chrono::steady_clock::time_point& t) {
    return std::chrono::duration<float>(nowClock() - t).count();
}

bool fileExists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

} // namespace

// Shader lookup order: environment override, working directory, then the
// source-tree path baked in by CMake. Covers running from the build tree,
// an installed prefix, or a debugger with an arbitrary CWD.
std::string Application::resolveShaderDir() {
    std::vector<std::string> candidates;

    if (const char* env = std::getenv("MAZE_SHADERS_DIR"))
        candidates.push_back(env);
    candidates.emplace_back("shaders");
#ifdef MAZE_SOURCE_SHADERS_DIR
    candidates.emplace_back(MAZE_SOURCE_SHADERS_DIR);
#endif

    for (const std::string& dir : candidates) {
        if (fileExists(dir + "/maze.vert")) return dir;
    }

    std::string tried;
    for (const std::string& dir : candidates) {
        tried += "  " + dir + "\n";
    }
    throw std::runtime_error("Application: shader files not found, looked in:\n" + tried);
}

Application::Application(const Options& options)
    : options_(options),
      window_(cfg::kInitialWidth, cfg::kInitialHeight, "Procedural Maze Explorer"),
      renderer_(resolveShaderDir()),
      seedRng_(std::random_device{}()) {
    const std::uint64_t seed = options_.seedExplicit ? options_.seed : nextSeed();
    maze_ = std::make_unique<Maze>(options_.cellsPerSide, seed);
    renderer_.build(*maze_);
    player_.spawnAtCell(*maze_,
                        options_.spawnAtExit ? maze_->exitCell() : maze_->startCell());
    minimapOn_ = options_.minimapOn;

    std::printf("Procedural Maze Explorer\n");
    std::printf("  maze grid : %dx%d (%d corridor cells per side)\n",
                maze_->gridSize(), maze_->gridSize(), maze_->cellsPerSide());
    std::printf("  seed      : %llu\n", static_cast<unsigned long long>(seed));
    std::printf("  controls  : WASD move, Shift sprint, mouse look,\n");
    std::printf("              M minimap, R new maze, [ ] resize, Esc quit\n");
}

std::uint64_t Application::nextSeed() { return seedRng_(); }

void Application::rebuildMaze(int cellsPerSide, std::uint64_t seed, bool fadeIn) {
    maze_->resize(cellsPerSide, seed);
    renderer_.build(*maze_);
    player_.spawnAtCell(*maze_, maze_->startCell());
    phase_ = Phase::Exploring;
    whiteFade_ = fadeIn ? 1.0f : 0.0f;

    std::printf("[maze] %dx%d grid, seed %llu\n", maze_->gridSize(), maze_->gridSize(),
                static_cast<unsigned long long>(seed));
}

void Application::handleInput() {
    InputState& in = window_.input();

    if (in.key(GLFW_KEY_ESCAPE)) window_.requestClose();
    if (in.keyPressed(GLFW_KEY_M)) minimapOn_ = !minimapOn_;
    if (in.keyPressed(GLFW_KEY_R))
        rebuildMaze(maze_->cellsPerSide(), nextSeed(), true);

    int delta = 0;
    if (in.keyPressed(GLFW_KEY_LEFT_BRACKET) || in.keyPressed(GLFW_KEY_MINUS))
        delta = -2;
    if (in.keyPressed(GLFW_KEY_RIGHT_BRACKET) || in.keyPressed(GLFW_KEY_EQUAL))
        delta = 2;
    if (delta != 0) {
        const int newSize = std::clamp(maze_->cellsPerSide() + delta,
                                       cfg::kMinCellsPerSide, cfg::kMaxCellsPerSide);
        if (newSize != maze_->cellsPerSide())
            rebuildMaze(newSize, nextSeed(), true);
    }
}

void Application::updateExitSequence(float dt) {
    switch (phase_) {
    case Phase::Exploring: {
        const Maze::Cell here =
            maze_->cellAtWorld(player_.position().x, player_.position().z);
        if (here == maze_->exitCell()) {
            phase_ = Phase::FadeOut;
            std::printf("[exit] beacon reached - escaping!\n");
        }
        break;
    }
    case Phase::FadeOut:
        whiteFade_ += dt / cfg::kWinFadeOutSeconds;
        if (whiteFade_ >= 1.0f) {
            rebuildMaze(maze_->cellsPerSide(), nextSeed(), false);
            phase_ = Phase::FadeIn;
        }
        break;
    case Phase::FadeIn:
        whiteFade_ -= dt / cfg::kWinFadeInSeconds;
        if (whiteFade_ <= 0.0f) {
            whiteFade_ = 0.0f;
            phase_ = Phase::Exploring;
        }
        break;
    }
}

void Application::renderFrame(double time) {
    // Torch flicker: layered sine waves give an organic, non-repeating wobble
    // without a noise texture or extra state.
    const float t = static_cast<float>(time);
    const float flicker = 0.5f * std::sin(t * 9.7f) +
                          0.3f * std::sin(t * 15.3f + 1.3f) +
                          0.2f * std::sin(t * 23.1f + 4.1f);
    const float torchPower = cfg::kTorchBasePower *
                             (1.0f + cfg::kTorchFlickerAmp * flicker);

    // Beacon breathes on its own; during the escape fade it flares up hard.
    const float beaconPulse = 0.85f + 0.25f * std::sin(t * cfg::kBeaconPulseSpeed);
    const float flare = (phase_ == Phase::FadeOut)
                            ? clampf(whiteFade_, 0.0f, 1.0f) * cfg::kWinBeaconFlare
                            : 0.0f;

    const auto [ex, ez] = maze_->cellCenter(maze_->exitCell());

    FrameState fs;
    fs.camera = &player_.camera();
    fs.viewportWidth = window_.framebufferWidth();
    fs.viewportHeight = window_.framebufferHeight();
    fs.torchPos = player_.camera().position;
    fs.torchPower = torchPower;
    fs.beaconPos = vec3(ex, cfg::kWallHeight * 0.42f, ez);
    fs.beaconPower = cfg::kBeaconBasePower * beaconPulse + flare;
    fs.showMinimap = minimapOn_;
    fs.whiteFade = whiteFade_;
    fs.time = time;

    renderer_.drawFrame(fs);
}

void Application::run() {
    auto lastFrame = nowClock();

    while (!window_.shouldClose()) {
        const float dt = std::min(secondsSince(lastFrame), 0.1f);
        lastFrame = nowClock();

        window_.pollEvents();
        handleInput();
        if (window_.shouldClose()) break;

        player_.update(*maze_, window_.input(), dt);
        updateExitSequence(dt);
        renderFrame(secondsSince(std::chrono::steady_clock::time_point{}));

        if (options_.shotEnabled && frameIndex_ == options_.shotFrames) {
            const bool ok = debug::saveScreenshotBMP(
                options_.shotPath, window_.framebufferWidth(),
                window_.framebufferHeight());
            std::printf("[shot] %s: %s\n", options_.shotPath.c_str(),
                        ok ? "written" : "FAILED");
            window_.requestClose();
        }

        window_.swapBuffers();
        window_.input().endFrame();
        ++frameIndex_;
    }
}

} // namespace maze
