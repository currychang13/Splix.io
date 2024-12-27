#include "splix_header.h"
int main()
{
    // signal(SIGWINCH, SIG_IGN); /* ignore window size changes */
    initscr();
    setlocale(LC_ALL, "");
    start_color();
    cbreak();             // disable line buffering, but allow signals(ctrl+c, ctrl+z, etc.)
    keypad(stdscr, TRUE); // enable function keys, arrow keys, etc. stdscr is the default window
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_WHITE, COLOR_BLACK);
    init_pair(4, COLOR_BLACK, COLOR_WHITE);
    init_pair(5, COLOR_BLUE, COLOR_BLACK);

    // windows
    while (true)
    {
        WINDOW *win1 = newwin(HEIGHT_WIN1, WIDTH_WIN1, (LINES - HEIGHT_WIN1) / 2, (COLS - WIDTH_WIN1) / 2); // size and location
        WINDOW *win2 = newwin(HEIGHT_WIN2, WIDTH_WIN2, (LINES - HEIGHT_WIN2) / 2, (COLS - WIDTH_WIN2) / 2);
        WINDOW *win3 = newwin(HEIGHT_WIN2 / 10, WIDTH_WIN2 / 4, (LINES - HEIGHT_WIN2) / 2, (COLS + WIDTH_WIN2) / 2);
        // show title
        box(win1, 0, 0);
        Rendertitle(win1);
        get_name_and_greet(win1);
        delwin(win1);
        clear();
        wrefresh(stdscr);

        // game loop
        noecho(); // disable displaying inputq
        game_loop(win2, win3);
        echo(); // enable displaying input again
        delwin(win2);
        delwin(win3);
        wrefresh(stdscr);

        // after game over
        /*mvwprintw(stdscr, 0, 0, "Game Over. Press 'r' to restart or 'q' to quit.");
        refresh();

        int ch = getch();
        if (ch == 'q')
        {
            break; // Exit the loop, ending the program
        }
        else if (ch != 'r')
        {
            break; // Default behavior is to exit unless 'r' is pressed
        }*/
    }
    endwin();
    return 0;
}
