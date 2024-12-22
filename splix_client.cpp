#include <curses.h>
#include <iostream>
#include <signal.h>
#include "unp.h"

using namespace std;
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
    mvwprintw(win, 1, width / 4 - 7, "Enter your name");
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
    char msg[100];
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
    int height_win1 = 10, width_win1 = 100;
    WINDOW *win1 = create_newwin(height_win1, width_win1, 0, 0); // size and location
    get_name_and_greet(win1);

    // game loop
    noecho(); // disable displaying inputq
    char ch;
    int height_win2 = 10, width_win2 = 100;
    WINDOW *win2 = create_newwin(height_win2, width_win2, 0, 0);
    while ((ch = wgetch(win2)))
    {
        wattron(win2, COLOR_PAIR(1) | A_BOLD);
        char str[50];
        (ch == 'q') ? sprintf(str, "See you again!\n") : sprintf(str, "You pressed %c\n", ch);
        mvwprintw(win2, 1, width_win2 / 2 - strlen(str) / 2, "%s", str);
        wattroff(win2, COLOR_PAIR(1) | A_BOLD);
        wrefresh(win2);
        if (ch == 'q')
            break;
    }
    echo(); // enable displaying input again
    sleep(1);
    endwin();
    return 0;
}