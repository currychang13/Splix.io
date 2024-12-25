#include <curses.h>
#include <iostream>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <queue>
#include "unp.h"
using namespace std;

// functions
WINDOW *create_newwin(int height, int width, int starty, int startx);
WINDOW *get_name_window(int height, int width);
void get_user_input(WINDOW *win, char *name);
void get_name_and_greet(WINDOW *win);
void fill_territory(int corner_y, int corner_x, WINDOW *win);
int check_territory(int corner_y, int corner_x, WINDOW *win);
void game_loop(WINDOW *win);
void initial_game();
void exit_game(WINDOW *win, int flag);

// size of windows
int height_win1 = 10, width_win1 = 100;
const int height_win2 = 30, width_win2 = 100;

// id allocate by server
int id = 1;
int map[height_win2][width_win2];

int main()
{
    // signal(SIGWINCH, SIG_IGN); /* ignore window size changes */
    initscr();
    start_color();
    cbreak();             // disable line buffering, but allow signals(ctrl+c, ctrl+z, etc.)
    keypad(stdscr, TRUE); // enable function keys, arrow keys, etc. stdscr is the default window
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_BLACK, COLOR_WHITE);
    // windows
    while (true)
    {
        initial_game();
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

// functions
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
void get_user_input(WINDOW *win, char *name)
{
    echo(); // enable displaying input
    keypad(win, TRUE);
    wmove(win, 1, 1);
    int ch, i = 0, cursor_position = 1;
    while ((ch = wgetch(win)) != '\n')
    {
        if (i == 0)
        {
            mvwprintw(win, 1, 1, "                ");
        }
        if (ch == KEY_BACKSPACE)
        {
            if (cursor_position > 1)
            {
                // Shift characters to the left
                for (int j = cursor_position - 1; j < i; j++)
                {
                    name[j - 1] = name[j];
                }

                i--;            // Reduce string length
                name[i] = '\0'; // Null-terminate the string

                cursor_position--;                 // Move cursor position back
                mvwprintw(win, 1, 1, "%s ", name); // Redraw string and clear last char
                wmove(win, 1, cursor_position);    // Set cursor position
                wrefresh(win);
            }
            else
            {
                wmove(win, 1, 1);
                continue;
            }
        }
        else if (ch == KEY_LEFT || ch == KEY_RIGHT || ch == KEY_UP || ch == KEY_DOWN)
        {
            switch (ch)
            {
            case KEY_LEFT:
                if (cursor_position > 1)
                    wmove(win, 1, --cursor_position);
                break;
            case KEY_RIGHT:
                if (cursor_position <= (int)strlen(name))
                    wmove(win, 1, ++cursor_position);
                break;
            default:
                break;
            }
        }
        else
        {
            for (int j = i; j >= cursor_position - 1; j--)
            {
                name[j + 1] = name[j];
            }
            name[cursor_position - 1] = ch;

            i++;               // Increase string length
            cursor_position++; // Move cursor position forward

            mvwprintw(win, 1, 1, "%s", name); // Redraw string
            wmove(win, 1, cursor_position);   // Set cursor position
            wrefresh(win);
        }
    }
    noecho(); // disable displaying input
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
    char name[50] = "";
    get_user_input(name_win, name);
    // after that, send to server
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
void create_initial_territory(WINDOW *win, int corner_y, int corner_x)
{
    // create initial territory
    // 1. create a rectangle
    // 2. fill the rectangle with the player's -id
    // 3. send the map to the server
    int dx[] = {0, 1, 0, -1};
    int dy[] = {1, 0, -1, 0};
    for (int i = 0; i < 4; i++)
    {
        int cur_x = corner_x + dx[i];
        for (int j = 0; j < 4; j++)
        {
            int cur_y = corner_y + dy[j];
            if (map[cur_y][cur_x] == -id || cur_x < 1 || cur_x > width_win2 - 2 || cur_y < 1 || cur_y > height_win2 - 2)
                continue;
            mvwprintw(win, cur_y, cur_x, "@");
            map[cur_y][cur_x] = -id;
        }
    }
    // send map to server
}
void fill_territory(int corner_y, int corner_x, WINDOW *win)
{
    // fill territory
    // flood fill algorithm
    // 1. check if the current cell is empty or not
    // 2. if empty, fill it and check the 4 adjacent cells
    // 3. if not empty, return
    queue<pair<int, int>> q;
    q.push({corner_y, corner_x});
    while (!q.empty())
    {
        pair<int, int> cur = q.front();
        q.pop();
        // check out of bound and if the cell is filled or not condition
        if (cur.first < 1 || cur.first > height_win2 - 2 || cur.second < 1 || cur.second > width_win2 - 2 || map[cur.first][cur.second] == -id)
            continue;
        // if the cell reaches the trail, fill it
        else if (map[cur.first][cur.second] == id)
        {
            map[cur.first][cur.second] = -id;
            mvwprintw(win, cur.first, cur.second, "@");
            continue;
        }
        map[cur.first][cur.second] = -id;
        mvwprintw(win, cur.first, cur.second, "@");
        q.push({cur.first + 1, cur.second});
        q.push({cur.first - 1, cur.second});
        q.push({cur.first, cur.second + 1});
        q.push({cur.first, cur.second - 1});
    }
    // send map to server
}
int check_territory(int corner_y, int corner_x, WINDOW *win)
{
    // three cases
    //  1. empty territory or other player's territory
    //  2. territory of the player(-id), fill territory
    //  3. trail of the player(id), die

    // touch the trail
    if (map[corner_y][corner_x] == id)
    {
        exit_game(win, 0); // die
        return 0;
    }
    // touch own territory, fill it
    if (map[corner_y][corner_x] == -id)
    {
        // fill territory
        fill_territory(corner_y, corner_x, win);
        return 1;
    }
    // empty territory or others' territory (normal case)
    map[corner_y][corner_x] = id;
    return 1;
}
void game_loop(WINDOW *win)
{
    keypad(win, TRUE);
    int ch; // use int to store the character like key_up, key_down, etc.

    // initial position
    int corner_x = 5, corner_y = 5;

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
    // create initial territory
    create_initial_territory(win, corner_y, corner_x);
    mvwprintw(win, corner_y, corner_x, "O"); // character
    wrefresh(win);

    nodelay(win, TRUE); // Non-blocking input

    // gaming
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
        // print trail
        mvwprintw(win, corner_y, corner_x, "#");

        // update position
        corner_y += direction.first;
        corner_x += direction.second;

        // check if the player is out of bound
        if (corner_y < 1 || corner_y >= height_win2 - 2 || corner_x < 1 || corner_x > width_win2 - 2)
        {
            exit_game(win, 0); // die
            return;
        }
        // die if the player dies touch his own trail
        if (!check_territory(corner_y, corner_x, win))
            return;

        mvwprintw(win, corner_y, corner_x, "O");
        wrefresh(win);
        usleep(200000); // Sleep for 0.1sec, speed of the game
    }
}
void initial_game()
{
    WINDOW *win1 = create_newwin(height_win1, width_win1, 0, 0); // size and location
    get_name_and_greet(win1);
    delwin(win1);
    // game loop
    noecho(); // disable displaying inputq
    WINDOW *win2 = create_newwin(height_win2, width_win2, 0, 0);
    game_loop(win2);
    echo(); // enable displaying input again
    delwin(win2);
}
void exit_game(WINDOW *win, int flag)
{
    // send to server
    for (int i = 0; i < height_win2; i++)
    {
        for (int j = 0; j < width_win2; j++)
        {
            if (map[i][j] == -id)
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
        return;
    }
    else // die
    {
        werase(win);
        box(win, 0, 0);
        mvwprintw(win, 1, width_win2 / 2 - 4, "You died!");
        wrefresh(win);
        sleep(2);
        werase(win);
        wrefresh(win);
        return;
    }
}