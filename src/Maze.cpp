#include "Maze.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

#include "Config.hpp"

namespace maze {

Maze::Maze(int cellsPerSide, std::uint64_t seed)
    : rng_(seed) {
    cellsPerSide_ = std::clamp(cellsPerSide, cfg::kMinCellsPerSide, cfg::kMaxCellsPerSide);
    gridSize_ = cellsPerSide_ * 2 + 1;
    generate();
}

void Maze::regenerate(std::uint64_t seed) {
    seed_ = seed;
    rng_.seed(seed);
    generate();
}

void Maze::resize(int cellsPerSide, std::uint64_t seed) {
    cellsPerSide_ = std::clamp(cellsPerSide, cfg::kMinCellsPerSide, cfg::kMaxCellsPerSide);
    gridSize_ = cellsPerSide_ * 2 + 1;
    regenerate(seed);
}

bool Maze::isWall(int row, int col) const {
    if (row < 0 || col < 0 || row >= gridSize_ || col >= gridSize_) return true;
    return grid_[static_cast<std::size_t>(row) * gridSize_ + col] != 0;
}

float Maze::worldMinX() const { return -0.5f * gridSize_ * cfg::kCellSize; }
float Maze::worldMinZ() const { return -0.5f * gridSize_ * cfg::kCellSize; }
float Maze::worldSpan() const { return gridSize_ * cfg::kCellSize; }

Maze::Cell Maze::cellAtWorld(float x, float z) const {
    const int col = static_cast<int>(std::floor((x - worldMinX()) / cfg::kCellSize));
    const int row = static_cast<int>(std::floor((z - worldMinZ()) / cfg::kCellSize));
    return Cell{row, col};
}

std::pair<float, float> Maze::cellCenter(Cell c) const {
    return { worldMinX() + (static_cast<float>(c.col) + 0.5f) * cfg::kCellSize,
             worldMinZ() + (static_cast<float>(c.row) + 0.5f) * cfg::kCellSize };
}

int Maze::openNeighbourCount(int row, int col) const {
    static constexpr std::array<std::pair<int, int>, 4> kDirs = {
        std::make_pair(-1, 0), std::make_pair(1, 0),
        std::make_pair(0, -1), std::make_pair(0, 1) };
    int count = 0;
    for (const auto& [dr, dc] : kDirs)
        if (!isWall(row + dr, col + dc)) ++count;
    return count;
}

// ---------------------------------------------------------------------------
// Recursive backtracker (iterative). Carves corridors two cells at a time so
// walls stay one cell thick.
// ---------------------------------------------------------------------------
void Maze::generate() {
    grid_.assign(static_cast<std::size_t>(gridSize_) * gridSize_, 1);

    auto cellIndex = [&](int row, int col) {
        return static_cast<std::size_t>(row) * gridSize_ + col;
    };

    constexpr int kDr[4] = { -2, 2, 0, 0 };
    constexpr int kDc[4] = { 0, 0, -2, 2 };

    start_ = Cell{ 1, 1 };
    grid_[cellIndex(1, 1)] = 0;

    std::vector<Cell> stack;
    stack.reserve(static_cast<std::size_t>(cellsPerSide_) * cellsPerSide_);
    stack.push_back(start_);

    while (!stack.empty()) {
        const Cell cur = stack.back();

        // Collect unvisited (still-solid) neighbours two cells away.
        int candidates[4];
        int count = 0;
        for (int i = 0; i < 4; ++i) {
            const int nr = cur.row + kDr[i];
            const int nc = cur.col + kDc[i];
            if (nr > 0 && nr < gridSize_ - 1 && nc > 0 && nc < gridSize_ - 1 &&
                grid_[cellIndex(nr, nc)] != 0) {
                candidates[count++] = i;
            }
        }

        if (count == 0) {
            stack.pop_back(); // dead end: backtrack
            continue;
        }

        std::uniform_int_distribution<int> pick(0, count - 1);
        const int dir = candidates[pick(rng_)];

        const int wallRow = cur.row + kDr[dir] / 2;
        const int wallCol = cur.col + kDc[dir] / 2;
        const int nextRow = cur.row + kDr[dir];
        const int nextCol = cur.col + kDc[dir];

        grid_[cellIndex(wallRow, wallCol)] = 0;
        grid_[cellIndex(nextRow, nextCol)] = 0;
        stack.push_back(Cell{ nextRow, nextCol });
    }

    braid();
    pickStartAndExit();
}

// ---------------------------------------------------------------------------
// Braiding: remove a fraction of the dead ends by opening one of their walls
// towards another corridor. Keeps the classic long backtracker corridors but
// adds loops, so a wrong turn never means a full backtrack.
// ---------------------------------------------------------------------------
void Maze::braid() {
    auto cellIndex = [&](int row, int col) {
        return static_cast<std::size_t>(row) * gridSize_ + col;
    };

    constexpr int kDr[4] = { -1, 1, 0, 0 };
    constexpr int kDc[4] = { 0, 0, -1, 1 };

    std::uniform_real_distribution<float> unit(0.0f, 1.0f);

    for (int row = 1; row < gridSize_ - 1; ++row) {
        for (int col = 1; col < gridSize_ - 1; ++col) {
            if (grid_[cellIndex(row, col)] != 0) continue;
            if (openNeighbourCount(row, col) != 1) continue;          // not a dead end
            if (unit(rng_) >= cfg::kBraidProbability) continue;

            int candidates[4];
            int count = 0;
            for (int i = 0; i < 4; ++i) {
                const int wallRow = row + kDr[i];
                const int wallCol = col + kDc[i];
                const int nextRow = row + 2 * kDr[i];
                const int nextCol = col + 2 * kDc[i];
                // Only knock through interior walls that lead somewhere open.
                if (nextRow > 0 && nextRow < gridSize_ - 1 &&
                    nextCol > 0 && nextCol < gridSize_ - 1 &&
                    grid_[cellIndex(wallRow, wallCol)] != 0 &&
                    grid_[cellIndex(nextRow, nextCol)] == 0) {
                    candidates[count++] = i;
                }
            }

            if (count > 0) {
                std::uniform_int_distribution<int> pick(0, count - 1);
                const int dir = candidates[pick(rng_)];
                grid_[cellIndex(row + kDr[dir], col + kDc[dir])] = 0;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// BFS from the start cell; the farthest corridor cell becomes the exit.
// ---------------------------------------------------------------------------
void Maze::pickStartAndExit() {
    start_ = Cell{ 1, 1 };

    std::vector<int> distance(static_cast<std::size_t>(gridSize_) * gridSize_, -1);
    auto cellIndex = [&](int row, int col) {
        return static_cast<std::size_t>(row) * gridSize_ + col;
    };

    std::vector<Cell> queue;
    queue.reserve(static_cast<std::size_t>(gridSize_) * gridSize_);
    queue.push_back(start_);
    distance[cellIndex(start_.row, start_.col)] = 0;

    constexpr int kDr[4] = { -1, 1, 0, 0 };
    constexpr int kDc[4] = { 0, 0, -1, 1 };

    std::size_t head = 0;
    Cell farthest = start_;
    int farthestDist = 0;

    while (head < queue.size()) {
        const Cell cur = queue[head++];
        const int curDist = distance[cellIndex(cur.row, cur.col)];

        if (curDist > farthestDist) {
            farthestDist = curDist;
            farthest = cur;
        }

        for (int i = 0; i < 4; ++i) {
            const int nr = cur.row + kDr[i];
            const int nc = cur.col + kDc[i];
            if (isWall(nr, nc)) continue;
            const std::size_t idx = cellIndex(nr, nc);
            if (distance[idx] != -1) continue;
            distance[idx] = curDist + 1;
            queue.push_back(Cell{ nr, nc });
        }
    }

    exit_ = farthest;
    exitDistance_ = farthestDist;
}

} // namespace maze
