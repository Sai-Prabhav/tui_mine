#include <algorithm> // for std::find
#include <chrono>
#include <csignal>
#include <cstdio>
#include <iostream>
#include <ncurses.h>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

struct Color {
    int red;
    int green;
    int blue;
};

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

typedef std::vector<std::vector<Cell>> Grid;

template <typename T>
bool in_vector(const std::vector<T> &vec,
               const T &val) {
    return std::find(vec.begin(), vec.end(), val) !=
           vec.end();
}

std::vector<Point> sample(int width, int height,
                          int num, Point point,
                          std::mt19937 gen) {

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

void sleep(int time) {
    std::this_thread::sleep_for(
        std::chrono::milliseconds(time));
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

Grid makeGrid(int width, int height, float bomb,
              std::mt19937 gen, Point point) {
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

int onClick(Grid &grid, Point point, int width,
            int height) {

    if (grid[point.x][point.y].bomb &&
        !grid[point.x][point.y].flagged) {
        return 1; // game lost
    } else if (!grid[point.x][point.y].clicked ||
               !grid[point.x][point.y].flagged) {
        expandGrid(grid, point.x, point.y, width,
                   height);
        return 0; // No problem
    }
    return 0;
}

void onSClick(Grid &grid, Point point) {
    if (!grid[point.x][point.y].clicked) {
        grid[point.x][point.y].flagged =
            !grid[point.x][point.y].flagged;
    }
}

void printch(char ch, int x, int y, int pair) {

    attron(COLOR_PAIR(pair));
    mvaddch(y, x, ch);
    attroff(COLOR_PAIR(pair));
}

void printchw(WINDOW *win, char ch, int x, int y,
              int pair) {

    wattron(win, COLOR_PAIR(pair));
    mvwaddch(win, y, x, ch);
    wattroff(win, COLOR_PAIR(pair));
}

int add_color(Color color) {

    static int color_index =
        8; // 0-7 index are standard colors
    init_color(++color_index, color.red, color.green,
               color.blue);
    return color_index;
}

int add_pair(int fg, int bg) {
    static int pair_index = 30;
    init_pair(++pair_index, fg, bg);
    return pair_index;
}

void drawCell(WINDOW *win, Cell cell, int x, int y) {
    static const int red = COLOR_RED;
    static const int green = COLOR_GREEN;
    static const int black = COLOR_BLACK;
    static const int white = COLOR_WHITE;
    static const int lgrey =
        add_color(Color{100, 100, 100});

    static const int grey =
        add_color(Color{500, 500, 500});
    static const int flagCol = add_pair(green, white);
    static const int valueCol = add_pair(red, grey);
    static const int bombCol = add_pair(red, black);
    static const int notClickedCol =
        add_pair(white, lgrey);
    static const int clickerCol = 1000;

    if (cell.flagged) {
        printchw(win, 'F', x, y, flagCol);
    } else if (!cell.clicked) {
        printchw(win, 'O', x, y, notClickedCol);
    } else if (cell.bomb) {
        printchw(win, 'X', x, y, 3);
    } else if (cell.value == 0) {
        printchw(win, ' ', x, y, clickerCol);
    } else {
        printchw(win, '0' + cell.value, x, y, valueCol);
    }
}

void drawGrid(WINDOW *win, Grid grid, int width,
              int height) {
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            drawCell(win, grid[i][j], i, j);
        }
    }
}

std::pair<Point, bool> getClick(WINDOW *win, Grid grid,
                                int width, int height) {
    static Point point{width / 2, height / 2};
    int ch;
    wmove(win, point.y, point.x);
    wrefresh(win);

    while (true) {

        drawGrid(win, grid, width, height);
        ch = getch(); // Get user input

        if (ch == 'q') {
            exit(0);
        } else if ((ch == 'w' || ch == KEY_UP) &&
                   point.y > 0)
            point.y--;
        else if ((ch == 's' || ch == KEY_DOWN) &&
                 point.y < height - 1)
            point.y++;
        else if ((ch == 'a' || ch == KEY_LEFT) &&
                 point.x > 0)
            point.x--;
        else if ((ch == 'd' || ch == KEY_RIGHT) &&
                 point.x < width - 1)
            point.x++;
        else if (ch == '\n') {
            return {point, false};

        } else if (ch == 'f') {
            return {point, true};
        }
        wmove(win, point.y, point.x);
        wrefresh(win);
    }
}
void showBombs(Grid &grid, int width, int height) {
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            grid[i][j].clicked = true;
        }
    }
}

int main() {

    // intilalise random
    std::random_device rd;
    std::mt19937 gen(rd());

    // initlalise ncurses

    initscr();
    cbreak();             // Disable line buffering
    noecho();             // Don't echo input
    keypad(stdscr, TRUE); // Enable arrow keys
    int height, width;
    getmaxyx(stdscr, height, width);
    height--;
    height--;
    width--;
    width--;
    start_color();

    // if (height < SIZE || width < SIZE) {
    //     printw("Make sure the screen's length and "
    //            "width are each at least %d",
    //            SIZE);
    //     getch();
    //     return 10;
    // }

    refresh();
    int x{width / 2}, y{height / 2};
    WINDOW *win = newwin(height, width, 1, 1);

    auto grid =
        makeGrid(width, height, 0.0f, gen, Point{1, 1});
    drawGrid(win, grid, width, height);
    wrefresh(win);
    auto [p, isFlag] =
        getClick(win, grid, width, height);
    grid = makeGrid(width, height, 0.1f, gen, p);
    expandGrid(grid, p.x, p.y, width, height);
    drawGrid(win, grid, width, height);
    wrefresh(win);

    bool isWin = true;
    while (true) {
        auto [p, isFlag] =
            getClick(win, grid, width, height);

        if (isFlag) {
            onSClick(grid, p);
        } else {
            if (onClick(grid, p, width, height) == 1) {
                isWin = false;
                move(0, (width / 2) - 4);
                printw("You Lost");
                showBombs(grid, width, height);
                drawGrid(win, grid, width, height);
                wrefresh(win);
                getch();
                break;
            }
        }

        drawGrid(win, grid, width, height);
        wmove(win, p.y, p.x);
        wrefresh(win);
    }

    delwin(win);
    endwin();
    return 0;
}

int ain() {
    initscr();
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
    refresh();
    WINDOW *win = newwin(10, 5, 1, 1);

    wattron(win,
            COLOR_PAIR(1)); // turn on red text for win
    mvwaddch(win, 2, 2, '#');
    wattroff(win, COLOR_PAIR(1)); // turn off red

    box(win, 0, 0);
    wrefresh(win);

    getch();
    endwin();
    return 0;
}
