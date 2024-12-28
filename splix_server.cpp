#include <curses.h>
#include <string.h>

int main()
{
    initscr();
    raw();
    noecho();
    curs_set(0);

    char *str = "Hello nigga";

    mvprintw(LINES/2, (COLS-strlen(str))/2, str);
    refresh();

    getch();
    endwin();
    return -1;
}