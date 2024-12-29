#ifndef SPLIX_HEADER_H
#define SPLIX_HEADER_H
#include <curses.h>
#include <iostream>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <queue>
#include <chrono>
#include <random>
#include <arpa/inet.h>
#include <string>
#include <vector>
#include <thread>

#define MAP_HEIGHT 600
#define MAP_WIDTH 600

// size of windows
#define HEIGHT_GAME_WIN 50
#define WIDTH_GAME_WIN 100

#define HEIGHT_INIT_WIN 50
#define WIDTH_INIT_WIN 100
#define name_length 20

#define acc_time 50
#define cool_time 50
#define speed 170000

enum GameStatus
{
    INITIAL,
    ROOM_SELECTION,
    GAMING,
    GAME_OVER
};
enum Mode
{
    NORMAL,
    FAST,
};

// id allocate by server
extern int id;
extern int map[MAP_HEIGHT][MAP_WIDTH];
extern Mode mode;
// classes

class Window
{
public:
    WINDOW *win;
    int height, width, starty, startx;
    Window(int h, int w, int starty, int startx) : height(h), width(w), starty(starty), startx(startx)
    {
        win = newwin(height, width, starty, startx);
    }
    ~Window()
    {
        delwin(win);
    }
    virtual void draw()
    {
        box(win, 0, 0);
        wrefresh(win);
    }
};

class Initial_Window : public Window
{
public:
    void Rendertitle();
    Initial_Window(int height, int width, int starty, int startx) : Window(height, width, starty, startx) {}
};

class Input_Window : public Window
{
public:
    char name[name_length] = "";
    void get_user_input();
    Input_Window(int height, int width, int starty, int startx) : Window(height, width, starty, startx) {}
    void draw() override
    {
        wattron(win, A_BOLD);
        box(win, 0, 0);
        wbkgd(win, COLOR_PAIR(20));
        mvwprintw(win, 1, 1, "Enter your name");
        wrefresh(win);
        wattroff(win, A_BOLD);
    }
};

class Status_Window : public Window
{
protected:
    int score = 0;

public:
    void update_status(int coordinate_y, int coordinate_x, const char *mode);
    void update_timer(int remain_time, int cooldown);
    Status_Window(int height, int width, int starty, int startx) : Window(height, width, starty, startx) {}
};

class Splix_Window : public Window
{
public:
    void draw() override
    {
        wattron(win, A_BOLD);
        //wborder(win, L'│', L'│', L'─', L'─', L'╭', L'╮', L'╰', L'╯');
        box(win, 0, 0);
        wrefresh(win);
        wattroff(win, A_BOLD);
    }
    Splix_Window(int height, int width, int starty, int startx) : Window(height, width, starty, startx) {}
    void create_initial_territory(int coordinate_y, int coordinate_x);
    void render_game(int coordinate_y, int coordinate_x);
    void exit_game(int flag);
    bool is_enclosure(int coordinate_y, int coordinate_x);
    std::vector<std::pair<int, int>> find_inside_points();
    void fill_territory(const std::vector<std::pair<int, int>> &inside_points);
    int check_valid_position(int coordinate_y, int coordinate_x);
};

class Room_Window : public Window
{
public:
    int selected_room = 0;
    Room_Window(int height, int width, int starty, int startx) : Window(height, width, starty, startx) {}
    void Renderroom();
    void select_room(std::vector<std::pair<int, int>> room_info);
};

class Gameover_Window : public Window
{
public:
    Gameover_Window(int height, int width, int starty, int startx) : Window(height, width, starty, startx) {}
    void render_gameover();
};

#endif