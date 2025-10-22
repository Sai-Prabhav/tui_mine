#include "minesweeper.h"
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <ncurses.h>

#include <random>
#include <thread>

struct Color {
    int red;
    int green;
    int blue;
};

void sleep(int time) {
    std::this_thread::sleep_for(
        std::chrono::milliseconds(time));
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

void printHeader(const char *str, int width) {
    char *space = new char[width + 1];
    std::memset(space, ' ', width);
    space[width] = '\0';

    mvprintw(0, 0, "%s", space);
    mvprintw(0, (width - std::strlen(str)) / 2, "%s",
             str);
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

void drawCell(WINDOW *win, Cell &cell, int x, int y) {
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
        if (ch == KEY_MOUSE) {
            MEVENT event;
            if (getmouse(&event) == OK) {

                point.x = event.x - 1;
                point.y = event.y - 1;
                wmove(win, point.y, point.x);
                wrefresh(win);
                if (event.bstate & BUTTON1_CLICKED)
                    return {point, false};
                else if (event.bstate & BUTTON3_CLICKED)
                    return {point, true};
            }
        } else if (ch == 'q') {
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

const float BOMB_RATIO = 0.1f;

int main() {

    // intilalise random
    std::random_device rd;
    std::mt19937 gen(rd());

    // initlalise ncurses

    initscr();
    cbreak();             // Disable line buffering
    noecho();             // Don't echo input
    keypad(stdscr, TRUE); // Enable arrow keys
    mousemask(ALL_MOUSE_EVENTS, NULL);

    int height, width;
    getmaxyx(stdscr, height, width);
    height -= 2;
    width -= 2;

    start_color();

    // if (height < SIZE || width < SIZE) {
    //     printw("Make sure the screen's length and "
    //            "width are each at least %d",
    //            SIZE);
    //     getch();
    //     return 10;
    // }

    refresh();
    WINDOW *win = newwin(height, width, 1, 1);

    auto grid =
        makeGrid(width, height, 0.0f, gen, Point{1, 1});
    drawGrid(win, grid, width, height);
    wrefresh(win);
    auto [p, isFlag] =
        getClick(win, grid, width, height);
    grid = makeGrid(width, height, BOMB_RATIO, gen, p);
    expandGrid(grid, p.x, p.y, width, height);
    drawGrid(win, grid, width, height);
    wrefresh(win);
    const int totalBomb = width * height * BOMB_RATIO;

    while (true) {
        auto [p, isFlag] =
            getClick(win, grid, width, height);

        if (isFlag) {
            onFlag(grid, p);
        } else {
            GameStatus status =
                onClick(grid, p, width, height);
            if (status == GAME_LOST) {
                printHeader("You Lost", width);
                showBombs(grid, width, height);
                drawGrid(win, grid, width, height);
                wrefresh(win);
                getch();
                break;
            } else if (status == GAME_WON) {
                printHeader("You Win", width);
                showBombs(grid, width, height);
                drawGrid(win, grid, width, height);
                wrefresh(win);
                getch();
                break;
            }
        }
        char hstr[16];
        std::snprintf(hstr, 16, "flags: %d/%d",
                      totalFlags(grid, width, height),
                      totalBomb);
        printHeader(hstr, width);
        drawGrid(win, grid, width, height);
        wmove(win, p.y, p.x);
        wrefresh(win);
        refresh();
    }

    delwin(win);
    endwin();
    return 0;
}
