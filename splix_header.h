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
#include <sstream>
#include <pthread.h>
#include <fstream>
#include <map>
#include <set>
#include <algorithm>

#define MAP_HEIGHT 100
#define MAP_WIDTH 100

#define SERVER_IP "127.0.0.1"

// size of windows
#define HEIGHT_INIT_WIN 50
#define WIDTH_INIT_WIN 100

#define HEIGHT_GAME_WIN 50
#define WIDTH_GAME_WIN 100

#define ROOM_LIMIT 15
#define name_length 20
#define id_length 4

#define acc_time 40
#define cool_time 40
#define speed 170000

#define BUFFER_SIZE 1024

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
    SLOW
};

extern int map[MAP_HEIGHT][MAP_WIDTH];

class Player
{
public:
    int coordinate_x;
    int coordinate_y;
    std::pair<int, int> direction;
    int id;
    int room_id;
    char name[name_length] = "";
    int score = 0;
    Mode mode = Mode::NORMAL;
    int acceleration_timer = acc_time;
    int cooldown_timer = 0;
    int occupy = -id;
    void init(std::pair<int, int> position, std::pair<int, int> direction, int id, Mode mode, int acceleration_timer, int cooldown_timer, int score);
};

class GameTicker
{
private:
    std::chrono::milliseconds frame_duration;
    std::chrono::steady_clock::time_point last_tick;

public:
    GameTicker(int fps) : frame_duration(1000 / fps), last_tick(std::chrono::steady_clock::now()) {}

    bool is_tick_due()
    {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_tick);

        if (elapsed >= frame_duration)
        {
            last_tick = now;
            return true;
        }
        return false;
    }
};

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
        wattron(win, A_BOLD);
        Custom_Border_2(win);
        wattroff(win, A_BOLD);
        wrefresh(win);
    }
    void clean()
    {
        werase(win);
        wrefresh(win);
    }
    void Custom_Border_1(WINDOW *win)
    {
        std::setlocale(LC_ALL, "");
        cchar_t vertical, horizontal, topLeft, topRight, bottomLeft, bottomRight;
        setcchar(&vertical, L"│", 0, 0, nullptr);
        setcchar(&horizontal, L"─", 0, 0, nullptr);
        setcchar(&topLeft, L"╭", 0, 0, nullptr);
        setcchar(&topRight, L"╮", 0, 0, nullptr);
        setcchar(&bottomLeft, L"╰", 0, 0, nullptr);
        setcchar(&bottomRight, L"╯", 0, 0, nullptr);

        wborder_set(win, &vertical, &vertical, &horizontal, &horizontal,
                    &topLeft, &topRight, &bottomLeft, &bottomRight);
    }
    void Custom_Border_2(WINDOW *win)
    {
        std::setlocale(LC_ALL, "");
        cchar_t vertical, horizontal, topLeft, topRight, bottomLeft, bottomRight;
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
    void Custom_Blink_Border(WINDOW *win, bool up, bool down, bool left, bool right)
    {
        std::setlocale(LC_ALL, ""); // Enable wide character support

        // Initialize cchar_t variables
        cchar_t vertical_left, vertical_right, horizontal_up, horizontal_down, topLeft, topRight, bottomLeft, bottomRight;

        // Set border components based on flags
        setcchar(&vertical_right, L"║", right ? A_BLINK : 0, 0, nullptr);
        setcchar(&vertical_left, L"║", left ? A_BLINK : 0, 0, nullptr);
        setcchar(&horizontal_up, L"═", up ? A_BLINK : 0, 0, nullptr);
        setcchar(&horizontal_down, L"═", down ? A_BLINK : 0, 0, nullptr);
        setcchar(&topLeft, L"╔", up || left ? A_BLINK : 0, 0, nullptr);
        setcchar(&topRight, L"╗", up || right ? A_BLINK : 0, 0, nullptr);
        setcchar(&bottomLeft, L"╚", down || left ? A_BLINK : 0, 0, nullptr);
        setcchar(&bottomRight, L"╝", down || right ? A_BLINK : 0, 0, nullptr);

        // Draw the custom border
        wborder_set(win, &vertical_left, &vertical_right, &horizontal_up, &horizontal_down,
                    &topLeft, &topRight, &bottomLeft, &bottomRight);
        // Refresh the window to display the changes
        wrefresh(win);
    }
};

class Initial_Window : public Window
{
public:
    void Rendertitle();
    void Show_Instruction();
    Initial_Window(int height, int width, int starty, int startx) : Window(height, width, starty, startx) {}
};

class Rule_Window : public Window
{
public:
    Rule_Window(int height, int width, int starty, int startx) : Window(height, width, starty, startx) {}
    void Show_Rules();
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
public:
    void display_player_status(const char *mode, Player player);
    void update_timer(int remain_time, int cooldown);
    Status_Window(int height, int width, int starty, int startx) : Window(height, width, starty, startx) {}
};

class Ranking_Window : public Window
{
public:
    Ranking_Window(int height, int width, int starty, int startx) : Window(height, width, starty, startx) {}
    void update_ranking(std::vector<Player> players);
};

class Map_Window : public Window
{
};
class Splix_Window : public Window
{
public:
    // int id;
    std::vector<std::vector<int>> previous_map;
    Splix_Window(int height, int width, int starty, int startx) : Window(height, width, starty, startx) {}
    void create_initial_territory(int coordinate_y, int coordinate_x, int id);
    void render_game(Player player);
    void exit_game(int flag);
    bool is_enclosure(int coordinate_y, int coordinate_x, int id);
    std::vector<std::pair<int, int>> find_inside_points(int id);
    void fill_players_territory(const std::vector<std::pair<int, int>> &inside_points, int id);
    std::vector<std::pair<int, int>> Heads;
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
    int selected_choice = 1;
    Create_Room_Window(int height, int width, int starty, int startx) : Window(height, width, starty, startx) {}
    void Render_create_room();
    void Show_Instruction();
    void Show_choice();
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

class UdpContent
{
public:
    int sockfd;
    int port;
    struct sockaddr_in servaddr;
    void udp_connect();
    void send_server_position(Player player);
    void send_leave_game();
    void get_initial_data(int &cli_id, std::pair<int, int> &position);
    void get_other_users_info(std::vector<Player> &players);
};

class TcpContent
{
public:
    int sockfd;
    int port = 12345;
    struct sockaddr_in servaddr;
    void tcp_connect();
    void send_server_name(char *name);
    void send_server_room_id(int room_id);
    void send_return_to_room_selection();
    void receive_room_info(std::vector<std::pair<int, int>> &room_info);
    void receive_member_info(std::vector<std::string> &member_info);
    void send_start();
    void receive_port(int &port);
};
#endif