#include "minesweeper.h"
#include <algorithm> // for std::find
#include <random>
#include <vector>
template <typename T>
bool in_vector(const std::vector<T> &vec,
               const T &val) {
    return std::find(vec.begin(), vec.end(), val) !=
           vec.end();
}

void addBomb(Grid &grid, int x, int y) {
    int width = grid.size();
    int height = grid[0].size();

    grid[x][y].bomb = true;
    if (x > 0) {
        if (y > 0)
            grid[x - 1][y - 1].value++;
        grid[x - 1][y].value++;
        if (y < (height - 1))
            grid[x - 1][y + 1].value++;
    }
    if (x < (width - 1)) {
        if (y > 0)
            grid[x + 1][y - 1].value++;
        grid[x + 1][y].value++;
        if (y < (height - 1))
            grid[x + 1][y + 1].value++;
    }
    if (y > 0)
        grid[x][y - 1].value++;

    if (y < (height - 1))
        grid[x][y + 1].value++;
}

std::vector<Point> sample(int width, int height,
                          int num, Point point,
                          std::mt19937 &gen) {

    int t = 0;
    int x, y;
    std::vector<Point> bombs(num);
    while (t < num) {
        x = gen() % width;
        y = gen() % height;
        if (!(abs(x - point.x) <= 1 ||
              abs(y - point.y) <= 1 ||
              in_vector(bombs, Point{x, y}))) {

            bombs[t++] = Point{x, y};
        }
    }
    return bombs;
}

Grid makeGrid(int width, int height, float bomb,
              std::mt19937 &gen, Point point) {
    auto bombs =
        sample(width, height,
               static_cast<int>(width * height * bomb),
               point, gen);

    Grid grid(width, std::vector<Cell>(
                         height,
                         Cell{0, false, false, false}));
    for (auto b : bombs) {
        addBomb(grid, b.x, b.y);
    }
    return grid;
}

void expandGrid(Grid &grid, int x, int y, int width,
                int height) {

    if (x < 0 || y < 0 || y >= height || x >= width)
        return;
    if (grid[x][y].bomb || grid[x][y].clicked) {
        return;
    }
    grid[x][y].clicked = true;
    if (grid[x][y].value == 0) {
        expandGrid(grid, x + 1, y, width, height);
        expandGrid(grid, x - 1, y, width, height);
        expandGrid(grid, x, y + 1, width, height);
        expandGrid(grid, x, y - 1, width, height);
        expandGrid(grid, x + 1, y + 1, width, height);
        expandGrid(grid, x - 1, y - 1, width, height);
        expandGrid(grid, x - 1, y + 1, width, height);
        expandGrid(grid, x + 1, y - 1, width, height);
    }
}

bool isCompleted(Grid &grid, int width, int height) {
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            if ((!grid[i][j].bomb) &&
                (!grid[i][j].clicked)) {
                return false;
            }
        }
    }
    return true;
}

GameStatus onClick(Grid &grid, Point point, int width,
                   int height) {
    if (grid[point.x][point.y].bomb &&
        !grid[point.x][point.y].flagged) {
        return GAME_LOST;
    } else if (!grid[point.x][point.y].clicked ||
               !grid[point.x][point.y].flagged) {
        expandGrid(grid, point.x, point.y, width,
                   height);
        if (isCompleted(grid, width, height)) {
            return GAME_WON;
        }
        return GAME_CONTINUE;
    }
    return GAME_CONTINUE;
}

void onFlag(Grid &grid, Point point) {
    if (!grid[point.x][point.y].clicked) {
        grid[point.x][point.y].flagged =
            !grid[point.x][point.y].flagged;
    }
}

void showBombs(Grid &grid, int width, int height) {
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            grid[i][j].clicked = true;
        }
    }
}

int totalFlags(Grid &grid, int width, int height) {
    int flag = 0;
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            flag += grid[i][j].flagged;
        }
    }
    return flag;
}
