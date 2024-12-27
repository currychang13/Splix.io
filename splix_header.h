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
#include "unp.h"

#define MAP_HEIGHT 600
#define MAP_WIDTH 600

// size of windows
#define HEIGHT_WIN2 50
#define WIDTH_WIN2 100

#define HEIGHT_WIN1 50
#define WIDTH_WIN1 100

// id allocate by server
const int id = 1;
int map[MAP_HEIGHT][MAP_WIDTH];

// functions
void render_game(WINDOW *win, int coordinate_y, int coordinate_x);
WINDOW *get_name_window();
void get_user_input(WINDOW *win, char *name);
void get_name_and_greet(WINDOW *win);
int check_valid_position(int coordinate_y, int coordinate_x, WINDOW *win);
void game_loop(WINDOW *win, WINDOW *status_win);
void exit_game(WINDOW *win, int flag);
void create_initial_territory(WINDOW *win, int coordinate_y, int coordinate_x);
void Rendertitle(WINDOW *win);
bool is_enclosure(int y, int x);
std::vector<std::pair<int, int>> find_inside_points();
void fill_territory(const std::vector<std::pair<int, int>> &inside_points);

#endif