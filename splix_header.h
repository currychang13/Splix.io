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
#define id_length 4

#define acc_time 50
#define cool_time 50
#define speed 170000

enum GameStatus
{
    INITIAL,
    ROOM_SELECTION,
    INSIDE_ROOM,
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
        Custom_Border_2(win);
        wrefresh(win);
    }
    void Custom_Border_1(WINDOW *win)
    {
        std::setlocale(LC_ALL, ""); // Ensure wide character support
        // Define cchar_t variables for border characters
        cchar_t vertical, horizontal, topLeft, topRight, bottomLeft, bottomRight;
        // Set wide characters directly
        setcchar(&vertical, L"│", 0, 0, nullptr);
        setcchar(&horizontal, L"─", 0, 0, nullptr);
        setcchar(&topLeft, L"╭", 0, 0, nullptr);
        setcchar(&topRight, L"╮", 0, 0, nullptr);
        setcchar(&bottomLeft, L"╰", 0, 0, nullptr);
        setcchar(&bottomRight, L"╯", 0, 0, nullptr);

        // Draw the custom border
        wborder_set(win, &vertical, &vertical, &horizontal, &horizontal,
                    &topLeft, &topRight, &bottomLeft, &bottomRight);
    }
    void Custom_Border_2(WINDOW *win)
    {
        std::setlocale(LC_ALL, ""); // Ensure wide character support
        // Define cchar_t variables for border characters
        cchar_t vertical, horizontal, topLeft, topRight, bottomLeft, bottomRight;
        // Set wide characters directly
        setcchar(&vertical, L"║", 0, 0, nullptr);
        setcchar(&horizontal, L"═", 0, 0, nullptr);
        setcchar(&topLeft, L"╔", 0, 0, nullptr);
        setcchar(&topRight, L"╗", 0, 0, nullptr);
        setcchar(&bottomLeft, L"╚", 0, 0, nullptr);
        setcchar(&bottomRight, L"╝", 0, 0, nullptr);

        // Draw the custom border
        wborder_set(win, &vertical, &vertical, &horizontal, &horizontal,
                    &topLeft, &topRight, &bottomLeft, &bottomRight);
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
    virtual void get_user_input();
    Input_Window(int height, int width, int starty, int startx) : Window(height, width, starty, startx) {}
    void draw() override
    {
        wattron(win, A_BOLD);
        wbkgd(win, COLOR_PAIR(20));
        Custom_Border_1(win);
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
    Splix_Window(int height, int width, int starty, int startx) : Window(height, width, starty, startx) {}
    void create_initial_territory(int coordinate_y, int coordinate_x);
    void render_game(int coordinate_y, int coordinate_x);
    void exit_game(int flag);
    bool is_enclosure(int coordinate_y, int coordinate_x);
    std::vector<std::pair<int, int>> find_inside_points();
    void fill_territory(const std::vector<std::pair<int, int>> &inside_points);
    int check_valid_position(int coordinate_y, int coordinate_x);
};

class Select_Room_Window : public Window
{
public:
    int selected_room = 0;
    Select_Room_Window(int height, int width, int starty, int startx) : Window(height, width, starty, startx) {}
    void Render_select_room();
    void select_room(std::vector<std::pair<int, int>> room_info);
};

class Create_Room_Window : public Window
{

public:
    Create_Room_Window(int height, int width, int starty, int startx) : Window(height, width, starty, startx) {}
    void Render_create_room();
};

class CR_Input_Window : public Input_Window
{
public:
    char id[id_length] = "";
    CR_Input_Window(int height, int width, int starty, int startx) : Input_Window(height, width, starty, startx) {}
    void get_user_input() override;
    void draw() override
    {
        wattron(win, A_BOLD);
        wbkgd(win, COLOR_PAIR(20));
        Custom_Border_1(win);
        mvwprintw(win, 1, 1, "Enter room id");
        wrefresh(win);
        wattroff(win, A_BOLD);
    }
};

class Room_Window : public Window
{
public:
    int selected_object = 0;
    Room_Window(int height, int width, int starty, int startx) : Window(height, width, starty, startx) {}
    void Render_room();
    void inside_room(std::vector<std::string> member_info, int room_id);
};
class Gameover_Window : public Window
{
public:
    Gameover_Window(int height, int width, int starty, int startx) : Window(height, width, starty, startx) {}
    void render_gameover();
};

#endif