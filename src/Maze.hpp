// Maze.hpp - grid-based maze representation and procedural generation.
//
// The maze lives on a (2n+1) x (2n+1) cell grid: corridor cells sit on odd
// coordinates, walls fill everything else. Generation is an iterative
// recursive-backtracker (iterative so large mazes cannot overflow the call
// stack), followed by a braiding pass that removes a fraction of the dead
// ends so the space contains loops and is more pleasant to explore.
//
// The exit is the corridor cell with the largest BFS distance from the start
// cell, which guarantees the player must cross most of the maze to reach it.
#pragma once

#include <cstdint>
#include <random>
#include <utility>
#include <vector>

namespace maze {

class Maze {
public:
    struct Cell {
        int row = 0;
        int col = 0;

        bool operator==(const Cell& o) const { return row == o.row && col == o.col; }
    };

    Maze(int cellsPerSide, std::uint64_t seed);

    // Rebuilds the maze in place with a new seed (same dimensions).
    void regenerate(std::uint64_t seed);
    // Changes the maze size and rebuilds it.
    void resize(int cellsPerSide, std::uint64_t seed);

    // Grid accessors. Out-of-range queries count as walls, which makes
    // border checks in collision code trivially safe.
    int  gridSize() const { return gridSize_; }          // 2n + 1
    int  cellsPerSide() const { return cellsPerSide_; }
    std::uint64_t seed() const { return seed_; }
    bool isWall(int row, int col) const;
    bool isWallCell(Cell c) const { return isWall(c.row, c.col); }
    const std::vector<std::uint8_t>& grid() const { return grid_; }

    // Grid <-> world mapping. World X grows with columns, world Z with rows;
    // the grid is centred on the origin. Cell positions are their centres.
    float worldMinX() const;
    float worldMinZ() const;
    float worldSpan() const; // full extent in metres (gridSize * cellSize)
    Cell  cellAtWorld(float x, float z) const;
    std::pair<float, float> cellCenter(Cell c) const;

    const Cell& startCell() const { return start_; }
    const Cell& exitCell() const { return exit_; }

    // Corridor cell whose BFS distance from the start is maximal.
    int exitDistance() const { return exitDistance_; }

private:
    void generate();
    void braid();
    void pickStartAndExit();
    int  openNeighbourCount(int row, int col) const;

    int cellsPerSide_ = 0;
    int gridSize_ = 0;
    std::uint64_t seed_ = 0;
    std::mt19937_64 rng_;
    std::vector<std::uint8_t> grid_; // 1 = wall, 0 = corridor
    Cell start_{};
    Cell exit_{};
    int exitDistance_ = 0;
};

} // namespace maze
