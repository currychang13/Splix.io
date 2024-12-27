#include <curses.h>
#include <iostream>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <queue>
#include <chrono>
#include "unp.h"
using namespace std;

#define map_height 600
#define map_width 600

// size of windows
#define height_win2 50
#define width_win2 50

#define height_win1 50
#define width_win1 50

// functions
void render_game(WINDOW *win, int coordinate_y, int coordinate_x);
WINDOW *create_newwin(int height, int width, int starty, int startx);
WINDOW *get_name_window(int height, int width);
void get_user_input(WINDOW *win, char *name);
void get_name_and_greet(WINDOW *win);
void fill_territory(int coordinate_y, int coordinate_x, WINDOW *win);
int check_valid_position(int coordinate_y, int coordinate_x, WINDOW *win);
void game_loop(WINDOW *win);
void initial_game();
void exit_game(WINDOW *win, int flag);
void create_initial_territory(WINDOW *win, int coordinate_y, int coordinate_x);

// id allocate by server
const int id = 1;
int map[map_height][map_width];

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

void render_game(WINDOW *win, int coordinate_y, int coordinate_x)
{
    int win_rows, win_cols;
    getmaxyx(win, win_rows, win_cols);

    // Determine half the window size
    int half_rows = win_rows / 2;
    int half_cols = win_cols / 2;

    // Calculate the top-left corner of the rendering area, relative to the window
    int start_y = coordinate_y - half_rows;
    int start_x = coordinate_x - half_cols;

    // Adjust the rendering area to stay within map boundaries
    if (start_y < 0)
        start_y = 0;
    if (start_x < 0)
        start_x = 0;
    if (start_y + win_rows > map_height)
        start_y = map_height - win_rows;
    if (start_x + win_cols > map_width)
        start_x = map_width - win_cols;

    // Clear the window and redraw the border
    werase(win);
    box(win, 0, 0);

    // Render the visible portion of the map
    for (int i = 0; i < win_rows - 2; i++)
    {
        for (int j = 0; j < win_cols - 2; j++)
        {
            int map_y = start_y + i;
            int map_x = start_x + j;

            // Skip out-of-bound map cells
            if (map_y < 0 || map_y >= map_height || map_x < 0 || map_x >= map_width)
                continue;

            // Determine the character to render based on map values
            char symbol;
            switch (map[map_y][map_x])
            {
            case 0:
                symbol = '.'; // Empty space
                wattron(win, COLOR_PAIR(3));
                break;
            case -id:
                symbol = '@'; // Filled territory
                wattron(win, COLOR_PAIR(id));
                break;
            case id:
                symbol = '#'; // Player trail
                wattron(win, COLOR_PAIR(id));
                break;
            default:
                symbol = '?'; // Undefined
                wattron(win, COLOR_PAIR(3));
                break;
            }

            // Render the character or cell
            if (map_y == coordinate_y && map_x == coordinate_x)
            {
                symbol = 'O'; // Player
            }

            mvwprintw(win, i + 1, j + 1, "%c", symbol);
            wattroff(win, COLOR_PAIR(3) | COLOR_PAIR(id));
        }
    }
    // Refresh the window after rendering
    wrefresh(win);
}
void Rendertitle(WINDOW *win)
{
    int maxY, maxX;
    getmaxyx(win, maxY, maxX);
    setlocale(LC_ALL, "");

    const wchar_t *title[] = {
        L"███████╗██████╗ ██╗     ██╗██╗  ██╗   ██╗ ██████╗ ",
        L"██╔════╝██╔══██╗██║     ██║╚██╗██╔╝   ██║██╔═══██╗",
        L"███████╗██████╔╝██║     ██║ ╚███╔╝    ██║██║   ██║",
        L"╚════██║██╔═══╝ ██║     ██║ ██╔██╗    ██║██║   ██║",
        L"███████║██║     ███████╗██║██╔╝ ██╗██╗██║╚██████╔╝",
        L"╚══════╝╚═╝     ╚══════╝╚═╝╚═╝  ╚═╝╚═╝╚═╝ ╚═════╝ ",
    };
    int artWidth = wcslen(title[0]);
    int startX = (maxX - artWidth) / 2;
    int artLines = sizeof(title) / sizeof(title[0]);
    int startY = 1;
    for (int i = 0; i < artLines; i++)
        mvwaddwstr(win, startY + i, startX, title[i]);
    wrefresh(win);
}
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
    int name_height = height / 15;
    int name_width = width - 4;
    WINDOW *win = newwin(name_height, name_width, 10, (COLS - name_width) / 2);
    box(win, 0, 0);
    wbkgd(win, COLOR_PAIR(4));
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
    // show title
    wattron(win, COLOR_PAIR(2) | A_BOLD);
    Rendertitle(win);
    wattroff(win, COLOR_PAIR(2) | A_BOLD);
    // create input window
    WINDOW *name_win = get_name_window(height_win1, width_win1);
    // get name
    char name[50] = "";
    get_user_input(name_win, name);
    // after that, send to server
    delwin(name_win);
}
void create_initial_territory(WINDOW *win, int coordinate_y, int coordinate_x)
{
    // create initial territory
    // 1. create a rectangle
    // 2. fill the rectangle with the player's -id
    // 3. send the map to the server
    int dx[] = {0, 1, 0, -1};
    int dy[] = {1, 0, -1, 0};
    for (int i = 0; i < 4; i++)
    {
        int cur_x = coordinate_x + dx[i];
        for (int j = 0; j < 4; j++)
        {
            int cur_y = coordinate_y + dy[j];
            if (map[cur_y][cur_x] == -id || cur_x < 1 || cur_x > width_win2 - 2 || cur_y < 1 || cur_y > height_win2 - 2)
                continue;
            map[cur_y][cur_x] = -id;
        }
    }
    render_game(win, coordinate_y, coordinate_x);
    // send map to server
}
pair<int, int> find_inside_point(int height, int width)
{
    vector<vector<bool>> visited(height, vector<bool>(width, false));
    queue<pair<int, int>> q;

    int dy[] = {-1, 1, 0, 0};
    int dx[] = {0, 0, -1, 1};

    // Mark all border and territory cells as "outside"
    for (int i = 0; i < height; i++)
    {
        for (int j : {0, width - 1})
        {
            if ((map[i][j] != 0 && map[i][j] != id) || visited[i][j])
            {
                visited[i][j] = true;
            }
            else
            {
                q.push({i, j});
                visited[i][j] = true;
            }
        }
    }
    for (int j = 0; j < width; j++)
    {
        for (int i : {0, height - 1})
        {
            if ((map[i][j] != 0 && map[i][j] != id) || visited[i][j])
            {
                visited[i][j] = true;
            }
            else
            {
                q.push({i, j});
                visited[i][j] = true;
            }
        }
    }

    // Flood fill from borders to mark outside points
    while (!q.empty())
    {
        auto [y, x] = q.front();
        q.pop();

        for (int d = 0; d < 4; d++)
        {
            int ny = y + dy[d];
            int nx = x + dx[d];

            if (ny < 0 || ny >= height || nx < 0 || nx >= width || visited[ny][nx])
                continue;

            if (map[ny][nx] != 0 && map[ny][nx] != id)
                continue;

            visited[ny][nx] = true;
            q.push({ny, nx});
        }
    }

    // Find the first unvisited point surrounded by the trail
    for (int y = 1; y < height - 1; y++)
    {
        for (int x = 1; x < width - 1; x++)
        {
            if (!visited[y][x] && map[y][x] == 0)
            { // Must be empty
                return {y, x};
            }
        }
    }

    return {-1, -1}; // No inside point found
}

void fill_territory(int coordinate_y, int coordinate_x, WINDOW *win)
{
    queue<pair<int, int>> q;
    q.push({coordinate_y, coordinate_x});

    int dy[] = {-1, 1, 0, 0};
    int dx[] = {0, 0, -1, 1};

    while (!q.empty())
    {
        auto [y, x] = q.front();
        q.pop();

        if (y < 0 || y >= map_height || x < 0 || x >= map_width || map[y][x] == -id)
            continue;

        if (map[y][x] == id)
        {
            map[y][x] = -id;
        }
        else
        {
            map[y][x] = -id;
            for (int d = 0; d < 4; d++)
            {
                q.push({y + dy[d], x + dx[d]});
            }
        }
    }

    render_game(win, coordinate_y, coordinate_x);
}

bool handle_fill_territory(WINDOW *win)
{
    // Find an inside point
    pair<int, int> inside_point = find_inside_point(map_height, map_width);
    if (inside_point.first == -1)
    {
        mvwprintw(win, 0, 0, "No enclosed area found!");
        wrefresh(win);
        return false;
    }

    // Fill the territory starting from the inside point
    fill_territory(inside_point.first, inside_point.second, win);
}
int check_valid_position(int coordinate_y, int coordinate_x, WINDOW *win)
{
    // three cases
    //  1. empty territory or other player's territory
    //  2. territory of the player(-id), fill territory
    //  3. trail of the player(id), die
    //  4. out of bound, die
    // out of bound or touch the trail -> die
    if (coordinate_y < 1 || coordinate_y >= map_height || coordinate_x < 1 || coordinate_x >= map_width || map[coordinate_y][coordinate_x] == id)
    {
        exit_game(win, 0); // die
        return 0;
    }
    return 1;
}
void game_loop(WINDOW *win)
{
    keypad(win, TRUE);
    int ch; // use int to store the character like key_up, key_down, etc.

    // initial position(coordinates)
    int coordinate_x = 5, coordinate_y = 5;

    // default direction
    pair<int, int> direction; // y and x
    direction.first = 0;
    direction.second = 1;

    // get map from server
    for (int i = 0; i < map_height; i++)
    {
        for (int j = 0; j < map_width; j++)
        {
            map[i][j] = 0;
        }
    }
    map[coordinate_y][coordinate_x] = id;

    wattron(win, COLOR_PAIR(1) | A_BOLD);
    // create initial territory
    create_initial_territory(win, coordinate_y, coordinate_x);

    nodelay(win, TRUE); // Non-blocking input
    render_game(win, coordinate_y, coordinate_x);
    // gaming
    for (;;)
    {
        if ((ch = wgetch(win)) != ERR)
        {
            switch (ch)
            {
            case 'q':
                wattroff(win, COLOR_PAIR(1) | A_BOLD);
                exit_game(win, 1);
                return; // exit the function
            case 'w':
            case KEY_UP:
                // if (coordinate_y == 1)
                // break; // break means exit the switch, not the loop,which ignores the operation
                direction.first = -1;
                direction.second = 0;
                break;
            case 's':
            case KEY_DOWN:
                // if (coordinate_y == height_win2 - 2)
                // break;
                direction.first = 1;
                direction.second = 0;
                break;
            case 'a':
            case KEY_LEFT:
                // if (coordinate_x == 1)
                // break;
                direction.first = 0;
                direction.second = -1;
                break;
            case 'd':
            case KEY_RIGHT:
                // if (coordinate_x == width_win2 - 2)
                // break;
                direction.first = 0;
                direction.second = 1;
                break;
            default:
                break;
            }
        }
        // update position

        coordinate_y += direction.first;
        coordinate_x += direction.second;

        // check if die or not
        if (!check_valid_position(coordinate_y, coordinate_x, win))
            return;
        // check the territory
        if (map[coordinate_y][coordinate_x] == -id)
        {
            // if circled, else, moving on its own territory
            handle_fill_territory(win);
        }
        else
        {
            map[coordinate_y][coordinate_x] = id;
        }
        render_game(win, coordinate_y, coordinate_x);
        usleep(200000); // Sleep for 0.1sec, speed of the game
    }
}
void initial_game()
{
    WINDOW *win1 = create_newwin(height_win1, width_win1, (LINES - width_win1) / 2, (COLS - height_win1) / 2); // size and location
    get_name_and_greet(win1);
    delwin(win1);
    clear();
    wrefresh(stdscr);
    // game loop
    noecho(); // disable displaying inputq
    WINDOW *win2 = create_newwin(height_win2, width_win2, (LINES - width_win2) / 2, (COLS - height_win2) / 2);
    game_loop(win2);
    echo(); // enable displaying input again
    delwin(win2);
    wrefresh(stdscr);
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