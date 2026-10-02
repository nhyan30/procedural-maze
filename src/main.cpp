// main.cpp - entry point: CLI parsing and top-level error handling.
//
// Usage:
//   maze [--size N] [--seed N] [--shot PATH] [--frames N] [--help]
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "Application.hpp"
#include "Config.hpp"

namespace {

void printUsage() {
    std::printf(
        "Procedural Maze Explorer - first-person OpenGL 3.3 maze demo\n"
        "\n"
        "Usage:\n"
        "  maze [options]\n"
        "\n"
        "Options:\n"
        "  --size N        corridor cells per side (grid is 2N+1), %d..%d, default %d\n"
        "  --seed N        deterministic maze seed (default: random each run)\n"
        "  --shot PATH     debug: render --frames N, write a BMP screenshot, exit\n"
        "  --frames N      frame count for --shot (default 30)\n"
        "  --at-exit       debug: spawn on the exit cell (tests the win sequence)\n"
        "  --minimap       start with the minimap overlay visible (default)\n"
        "  --no-minimap    start without the minimap overlay (M toggles it)\n"
        "  --help          show this message\n"
        "\n"
        "Controls:\n"
        "  WASD / mouse    move and look (Shift sprints)\n"
        "  M               toggle minimap\n"
        "  R               regenerate the maze\n"
        "  [ and ]         shrink / grow the maze\n"
        "  Esc             quit\n",
        maze::cfg::kMinCellsPerSide, maze::cfg::kMaxCellsPerSide,
        maze::cfg::kDefaultCellsPerSide);
}

bool parseArgs(int argc, char** argv, maze::Options& options, bool& showHelp) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        // Accept both "--opt value" and "--opt=value".
        std::string inlineValue;
        bool hasInline = false;
        if (arg.rfind("--", 0) == 0) {
            const std::size_t eq = arg.find('=');
            if (eq != std::string::npos) {
                inlineValue = arg.substr(eq + 1);
                arg = arg.substr(0, eq);
                hasInline = true;
            }
        }

        auto value = [&]() -> const char* {
            if (hasInline) return inlineValue.c_str();
            if (i + 1 >= argc) return nullptr;
            return argv[++i];
        };

        try {
            if (arg == "--help" || arg == "-h") {
                showHelp = true;
            } else if (arg == "--size" || arg == "-s") {
                const char* v = value();
                if (!v) { std::fprintf(stderr, "error: %s needs a value\n", arg.c_str()); return false; }
                options.cellsPerSide = std::clamp(std::atoi(v),
                                                  maze::cfg::kMinCellsPerSide,
                                                  maze::cfg::kMaxCellsPerSide);
            } else if (arg == "--seed") {
                const char* v = value();
                if (!v) { std::fprintf(stderr, "error: %s needs a value\n", arg.c_str()); return false; }
                options.seed = std::strtoull(v, nullptr, 10);
                options.seedExplicit = true;
            } else if (arg == "--shot" || arg == "--screenshot") {
                const char* v = value();
                if (!v) { std::fprintf(stderr, "error: %s needs a value\n", arg.c_str()); return false; }
                options.shotPath = v;
                options.shotEnabled = true;
            } else if (arg == "--at-exit") {
                options.spawnAtExit = true;
            } else if (arg == "--minimap") {
                options.minimapOn = true;
            } else if (arg == "--no-minimap") {
                options.minimapOn = false;
            } else if (arg == "--frames") {
                const char* v = value();
                if (!v) { std::fprintf(stderr, "error: %s needs a value\n", arg.c_str()); return false; }
                options.shotFrames = std::max(1, std::atoi(v));
            } else {
                std::fprintf(stderr, "error: unknown option '%s'\n", argv[i]);
                return false;
            }
        } catch (const std::exception& e) {
            std::fprintf(stderr, "error: bad value for %s: %s\n", arg.c_str(), e.what());
            return false;
        }
    }
    return true;
}

} // namespace

int main(int argc, char** argv) {
    maze::Options options;
    bool showHelp = false;

    if (!parseArgs(argc, argv, options, showHelp)) {
        std::fprintf(stderr, "\n");
        printUsage();
        return 1;
    }
    if (showHelp) {
        printUsage();
        return 0;
    }

    try {
        maze::Application app(options);
        app.run();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "fatal: %s\n", e.what());
        return 1;
    }
    return 0;
}
