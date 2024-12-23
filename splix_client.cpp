#include <curses.h>
#include <iostream>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "unp.h"

using namespace std;
// size of windows
int height_win1 = 10, width_win1 = 100;
const int height_win2 = 30, width_win2 = 100;
// id allocate by server
int id = 1;
int map[height_win2][width_win2];
WINDOW *create_newwin(int height, int width, int starty, int startx)
{
    // int yMax, xMax;
    // getmaxyx(stdscr, yMax, xMax);                      // get the dimensions of the window
    WINDOW *win1 = newwin(height, width, starty, startx); // height, width, y, x
    box(win1, 0, 0);                                      // draw a box around the window
    wrefresh(win1);
    return win1;
}
WINDOW *get_name_window(int height, int width)
{
    WINDOW *win = newwin(height / 3, width / 2, 2, width / 2 - width / 4);
    box(win, 0, 0);
    wbkgd(win, COLOR_PAIR(3));
    mvwprintw(win, 1, 1, "Enter your name");
    wrefresh(win);
    return win;
}
void get_name_and_greet(WINDOW *win)
{
    // get window size
    int rows, cols;
    getmaxyx(win, rows, cols);
    // show title
    wattron(win, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(win, 1, cols / 2 - 4, "Splix.io");
    wattroff(win, COLOR_PAIR(2) | A_BOLD);
    wrefresh(win);
    // create input window
    WINDOW *name_win = get_name_window(rows, cols);
    // get name
    char name[50];
    mvwscanw(name_win, 1, 1, "%50s", name); // after that, send to server
    delwin(name_win);
    // greet
    /*char msg[100];
    sprintf(msg, "Welcome to Splix.io %s!", name);
    werase(win);
    box(win, 0, 0);
    wattron(win, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(win, 1, cols / 2 - strlen(msg) / 2, "%s", msg);
    wrefresh(win);
    sleep(1);
    mvwprintw(win, 2, cols / 2 - 15, "WASD to move, or 'q' to quit.");
    wrefresh(win);
    sleep(2);
    mvwprintw(win, rows - 2, cols / 2 - 15, "Press any key to continue...");
    wattroff(win, COLOR_PAIR(2) | A_BOLD);
    wgetch(win); // wait for next character, or it will skip the greeting msg
    */
}
void check_valid_move(int &corner_y, int &corner_x)
{
    if (corner_y < 1)
        corner_y = 1;
    if (corner_y > height_win2 - 2)
        corner_y = height_win2 - 2;
    if (corner_x < 1)
        corner_x = 1;
    if (corner_x > width_win2 - 2)
        corner_x = width_win2 - 2;
}

void exit_game(WINDOW *win, int flag)
{
    // send to server
    for (int i = 0; i < height_win2; i++)
    {
        for (int j = 0; j < width_win2; j++)
        {
            if (map[i][j] == id)
            {
                map[i][j] = 0;
            }
        }
    }
    // animation
    if (flag == 1) // normal exit
    {
        // animation
        char msg[20];
        for (int i = 0; i < 3; i++)
        {
            for (int j = 3; j >= 0; j--)
            {
                werase(win);
                box(win, 0, 0);
                snprintf(msg, 11 - j, "Exiting...");
                mvwprintw(win, 1, width_win2 / 2 - 4, "%s", msg);
                wrefresh(win);
                usleep(500000);
            }
        }
    }
    else // die
    {
        werase(win);
        box(win, 0, 0);
        mvwprintw(win, 1, width_win2 / 2 - 4, "You died!");
        wrefresh(win);
        sleep(1);
    }
}
void check_territory(int corner_y, int corner_x, WINDOW *win)
{
    if (map[corner_y][corner_x] == id)
    {
        exit_game(win, 0); // die
    }
    else // fill the territory
    {
    }
}
void game_loop(WINDOW *win)
{
    char ch;
    int corner_x = 1, corner_y = 1;
    // default direction
    pair<int, int> direction; // y and x
    direction.first = 0;
    direction.second = 1;
    // get map from server
    for (int i = 0; i < height_win2; i++)
    {
        for (int j = 0; j < width_win2; j++)
        {
            map[i][j] = 0;
        }
    }
    map[corner_y][corner_x] = id;

    wattron(win, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(win, corner_y, corner_x, "O"); // character
    wrefresh(win);
    nodelay(win, TRUE); // Non-blocking input
    for (;;)
    {
        if ((ch = wgetch(win)))
        {
            switch (ch)
            {
            case 'q':
                wattroff(win, COLOR_PAIR(1) | A_BOLD);
                exit_game(win, 1);
                return; // exit the function
            case 'w':
            case KEY_UP:
                if (corner_y == 1)
                    break; // break means exit the switch, not the loop,which ignores the operation
                direction.first = -1;
                direction.second = 0;
                break;
            case 's':   
            case KEY_DOWN:
                if (corner_y == height_win2 - 2)
                    break;
                direction.first = 1;
                direction.second = 0;
                break;
            case 'a':
            case KEY_LEFT:
                if (corner_x == 1)
                    break;
                direction.first = 0;
                direction.second = -1;
                break;
            case 'd':
            case KEY_RIGHT:
                if (corner_x == width_win2 - 2)
                    break;
                direction.first = 0;
                direction.second = 1;
                break;
            default:
                break;
            }
        }
        mvwprintw(win, corner_y, corner_x, "#");
        corner_y += direction.first;
        corner_x += direction.second;

        check_valid_move(corner_y, corner_x);
        //check_territory(corner_y, corner_x, win);

        mvwprintw(win, corner_y, corner_x, "O");
        map[corner_y][corner_x] = id;
        wrefresh(win);

        usleep(100000); // Sleep for 0.1sec, speed of the game
    }
}
int main()
{
    signal(SIGWINCH, SIG_IGN); /* ignore window size changes */
    initscr();
    start_color();
    cbreak();             // disable line buffering, but allow signals(ctrl+c, ctrl+z, etc.)
    keypad(stdscr, TRUE); // enable function keys, arrow keys, etc. stdscr is the default window
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_BLACK, COLOR_WHITE);
    // windows

    WINDOW *win1 = create_newwin(height_win1, width_win1, 0, 0); // size and location
    get_name_and_greet(win1);

    // game loop
    noecho(); // disable displaying inputq
    WINDOW *win2 = create_newwin(height_win2, width_win2, 0, 0);
    keypad(win2, TRUE);
    game_loop(win2);
    echo(); // enable displaying input again
    sleep(1);
    endwin();
    return 0;
}