
#pragma once

#include <cstdint>
#include <random>
#include <vector>

// Forward declarations

struct Cell {
    uint8_t value : 4;
    bool clicked : 1;
    bool flagged : 1;
    bool bomb : 1;
};

struct Point {
    int x;
    int y;

    bool operator==(const Point &other) const {
        return x == other.x && y == other.y;
    }
};

using Grid = std::vector<std::vector<Cell>>;

enum GameStatus { GAME_CONTINUE, GAME_WON, GAME_LOST };

// Forward declare functions

template <typename T>
bool in_vector(const std::vector<T> &vec, const T &val);

void addBomb(Grid &grid, int x, int y);

std::vector<Point> sample(int width, int height,
                          int num, Point point,
                          std::mt19937 &gen);

Grid makeGrid(int width, int height, float bomb,
              std::mt19937 &gen, Point point);

void expandGrid(Grid &grid, int x, int y, int width,
                int height);

bool isCompleted(Grid &grid, int width, int height);

GameStatus onClick(Grid &grid, Point point, int width,
                   int height);

void onFlag(Grid &grid, Point point);

void showBombs(Grid &grid, int width, int height);

int totalFlags(Grid &grid, int width, int height);
