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
void get_name_and_greet(WINDOW *win1)
{
    char name[50];
    wattron(win1, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(win1, 1, 1, "Enter your name: ");
    wattroff(win1, COLOR_PAIR(2) | A_BOLD);
    wrefresh(win1);

    mvwscanw(win1, 2, 1, "%s", name);

    wattron(win1, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(win1, 3, 1, "Hello, %s!", name);
    mvwprintw(win1, 4, 1, "Press any key, or 'q' to quit.");
    wattroff(win1, COLOR_PAIR(2) | A_BOLD);
    wrefresh(win1);
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

    WINDOW *win1 = create_newwin(10, 50, 0, 0);
    get_name_and_greet(win1);
    
    noecho(); // disable displaying inputq
    char ch;
    //game loop
    while ((ch = wgetch(win1)))
    {
        wattron(win1, COLOR_PAIR(1) | A_BOLD);
        char str[50];
        (ch == 'q') ? sprintf(str, "Goodbye!") : sprintf(str, "You pressed %c", ch);
        mvwprintw(win1, 5, 1, "%s\n", str);
        wattroff(win1, COLOR_PAIR(1) | A_BOLD);
        wrefresh(win1);
        if (ch == 'q')
            break;
    }
    echo(); // enable displaying input again
    sleep(1);
    endwin();
    return 0;
}