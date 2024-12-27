#include "splix_header.h"

// functions
// render functions
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
    if (start_y + win_rows > MAP_HEIGHT)
        start_y = MAP_HEIGHT - win_rows;
    if (start_x + win_cols > MAP_WIDTH)
        start_x = MAP_WIDTH - win_cols;

    // Clear the window and redraw the border
    setlocale(LC_ALL, "");
    // Render the visible portion of the map
    for (int i = 0; i < win_rows - 2; i++)
    {
        for (int j = 0; j < win_cols - 2; j++)
        {
            int map_y = start_y + i;
            int map_x = start_x + j;

            // Skip out-of-bound map cells
            if (map_y < 0 || map_y >= MAP_HEIGHT || map_x < 0 || map_x >= MAP_WIDTH)
                continue;

            // Determine the character to render based on map values
            ;
            const wchar_t *symbol;
            switch (map[map_y][map_x])
            {
            case 0:
                symbol = L"."; // Empty space
                wattron(win, COLOR_PAIR(5));
                break;
            case -id:
                symbol = L"■"; // Filled territory
                wattron(win, COLOR_PAIR(id));
                break;
            case id:
                symbol = L"▪"; // Player trail
                wattron(win, COLOR_PAIR(id));
                break;
            default:
                symbol = L"?"; // Undefined
                wattron(win, COLOR_PAIR(3));
                break;
            }

            // Render the character or cell
            if (map_y == coordinate_y && map_x == coordinate_x)
            {
                symbol = L"O"; // Player
            }
            mvwaddwstr(win, i + 1, j + 1, symbol);
            wattroff(win, COLOR_PAIR(3) | COLOR_PAIR(id));
        }
    }
    // Refresh the window after rendering
    wrefresh(win);
}
void Rendertitle(WINDOW *win)
{
    wattron(win, COLOR_PAIR(1) | A_BOLD);
    int maxX = getmaxx(win);
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
    int startY = 3;
    for (int i = 0; i < artLines; i++)
        mvwaddwstr(win, startY + i, startX, title[i]);
    wrefresh(win);
    wattroff(win, COLOR_PAIR(1) | A_BOLD);
}

// window functions

WINDOW *get_name_window()
{
    int name_height = HEIGHT_WIN1 / 12;
    int name_width = WIDTH_WIN2 / 2;
    WINDOW *name_win = newwin(name_height, name_width, (HEIGHT_WIN1 - name_height) / 2.5, (COLS - name_width) / 2);
    wattron(name_win, A_BOLD);
    box(name_win, 0, 0);
    wbkgd(name_win, COLOR_PAIR(4));
    mvwprintw(name_win, 1, 1, "Enter your name");
    wrefresh(name_win);
    wattroff(name_win, A_BOLD);
    return name_win;
}

// input functions
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

// game functions
void update_status(WINDOW *win, int coordinate_y, int coordinate_x)
{
    // Update the status window
    wattron(win, A_BOLD);
    mvwprintw(win, 1, 1, "Status");
    // get status from server
    int score = 0;
    for (int i = 0; i < MAP_HEIGHT; i++)
    {
        for (int j = 0; j < MAP_WIDTH; j++)
        {
            if (map[i][j] == -id)
            {
                score++;
            }
        }
    }
    mvwprintw(win, 2, 1, "Score: %d", score);
    mvwprintw(win, 3, 1, "Position: (%d, %d)", coordinate_y, coordinate_x);
    wrefresh(win);
    wattroff(win, A_BOLD);
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
            if (map[cur_y][cur_x] == -id || cur_x < 0 || cur_x >= MAP_WIDTH || cur_y < 0 || cur_y >= MAP_HEIGHT)
                continue;
            map[cur_y][cur_x] = -id;
        }
    }
    render_game(win, coordinate_y, coordinate_x);
    // send map to server
}
bool is_enclosure(int y, int x)
{
    // BFS to determine if the trail forms a closed boundary
    std::vector<std::vector<bool>> visited(MAP_HEIGHT, std::vector<bool>(MAP_WIDTH, false));
    std::queue<std::pair<int, int>> q;

    // Start from the current position
    q.push({y, x});
    visited[y][x] = true;

    bool touches_border = false;

    int dy[] = {-1, 1, 0, 0};
    int dx[] = {0, 0, -1, 1};

    while (!q.empty())
    {
        auto [cy, cx] = q.front();
        q.pop();

        for (int d = 0; d < 4; d++)
        {
            int ny = cy + dy[d];
            int nx = cx + dx[d];

            // Out-of-bounds check
            if (ny < 0 || ny >= MAP_HEIGHT || nx < 0 || nx >= MAP_WIDTH)
            {
                touches_border = true;
                continue;
            }

            // Skip visited cells
            if (visited[ny][nx])
                continue;

            // Only consider trail (`id`) and territory (`-id`)
            if (map[ny][nx] == id || map[ny][nx] == -id)
            {
                visited[ny][nx] = true;
                q.push({ny, nx});
            }
        }
    }

    // If the component touches the border, it's not an enclosure
    return !touches_border;
}
std::vector<std::pair<int, int>> find_inside_points()
{
    std::vector<std::vector<bool>> visited(MAP_HEIGHT, std::vector<bool>(MAP_WIDTH, false));
    std::queue<std::pair<int, int>> q;

    int dy[] = {-1, 1, 0, 0};
    int dx[] = {0, 0, -1, 1};

    // Mark all border-connected areas as "outside"
    for (int i = 0; i < MAP_HEIGHT; i++)
    {
        for (int j : {0, MAP_WIDTH - 1})
        {
            if (map[i][j] != 0 /*&& map[i][j] != id*/)
            {
                visited[i][j] = true; // Mark borders and filled territory
            }
            else if (!visited[i][j])
            {
                q.push({i, j});
                visited[i][j] = true;
            }
        }
    }
    for (int j = 0; j < MAP_WIDTH; j++)
    {
        for (int i : {0, MAP_HEIGHT - 1})
        {
            if (map[i][j] != 0 /*&& map[i][j] != id*/)
            {
                visited[i][j] = true; // Mark borders and filled territory
            }
            else if (!visited[i][j])
            {
                q.push({i, j});
                visited[i][j] = true;
            }
        }
    }

    // Flood-fill to mark all "outside" cells
    while (!q.empty())
    {
        auto [y, x] = q.front();
        q.pop();

        for (int d = 0; d < 4; d++)
        {
            int ny = y + dy[d];
            int nx = x + dx[d];

            if (ny < 0 || ny >= MAP_HEIGHT || nx < 0 || nx >= MAP_WIDTH || visited[ny][nx])
                continue;
            // other wise, mark as visited
            if (map[ny][nx] != id && map[ny][nx] != -id)
            {
                visited[ny][nx] = true;
                q.push({ny, nx});
            }
        }
    }

    // Collect all unvisited empty cells as inside points
    std::vector<std::pair<int, int>> inside_points;
    for (int y = 1; y < MAP_HEIGHT - 1; y++)
    {
        for (int x = 1; x < MAP_WIDTH - 1; x++)
        {
            if (!visited[y][x] && (map[y][x] == 0 || map[y][x] == id))
            {
                inside_points.push_back({y, x});
            }
        }
    }

    return inside_points;
}
void fill_territory(const std::vector<std::pair<int, int>> &inside_points)
{
    for (const auto &[y, x] : inside_points)
    {
        map[y][x] = -id; // Mark as filled territory
    }
}
int check_valid_position(int coordinate_y, int coordinate_x, WINDOW *win)
{
    // three cases
    //  1. empty territory or other player's territory
    //  2. territory of the player(-id), fill territory
    //  3. trail of the player(id), die
    //  4. out of bound, die
    // out of bound or touch the trail -> die
    if (coordinate_y < 1 || coordinate_y >= MAP_HEIGHT || coordinate_x < 1 || coordinate_x >= MAP_WIDTH || map[coordinate_y][coordinate_x] == id)
    {
        exit_game(win, 0); // die
        return 0;
    }
    return 1;
}

// flow functions
void get_name_and_greet(WINDOW *win)
{
    // create input window
    WINDOW *name_win = get_name_window();
    // get name
    char name[50] = "";
    get_user_input(name_win, name);
    // after that, send to server
    delwin(name_win);
}
void game_loop(WINDOW *win, WINDOW *status_win)
{
    // Disable the cursor
    curs_set(0);
    keypad(win, TRUE);
    nodelay(win, TRUE); // Non-blocking input
    wattron(win, COLOR_PAIR(1) | A_BOLD);

    // get map from server
    for (int i = 0; i < MAP_HEIGHT; i++)
    {
        for (int j = 0; j < MAP_WIDTH; j++)
        {
            map[i][j] = 0;
        }
    }

    // initial position(coordinates), receive from server
    srand(time(NULL));
    int coordinate_x = rand() % MAP_WIDTH, coordinate_y = rand() % MAP_HEIGHT;
    map[coordinate_y][coordinate_x] = id;

    // default direction
    std::pair<int, int> direction; // y and x
    direction.first = 0;
    direction.second = 1;

    // create initial territory
    create_initial_territory(win, coordinate_y, coordinate_x);
    render_game(win, coordinate_y, coordinate_x);
    update_status(status_win, coordinate_y, coordinate_x);
    // gaming
    int ch; // use int to store the character like key_up, key_down, etc.
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
                direction.first = -1;
                direction.second = 0;
                break; // break means exit the switch, not the loop,which ignores the operation
            case 's':
            case KEY_DOWN:

                direction.first = 1;
                direction.second = 0;
                break;
            case 'a':
            case KEY_LEFT:

                direction.first = 0;
                direction.second = -1;
                break;
            case 'd':
            case KEY_RIGHT:
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
            if (is_enclosure(coordinate_y, coordinate_x))
            {
                // Find all inside points
                auto inside_points = find_inside_points();

                // Fill the enclosed area
                fill_territory(inside_points);
            }
        }
        else
        {
            map[coordinate_y][coordinate_x] = id;
        }
        render_game(win, coordinate_y, coordinate_x);
        update_status(status_win, coordinate_y, coordinate_x);
        usleep(200000); // Sleep for 0.1sec, speed of the game
    }
}

void exit_game(WINDOW *win, int flag)
{
    // send to server
    for (int i = 0; i < HEIGHT_WIN2; i++)
    {
        for (int j = 0; j < WIDTH_WIN2; j++)
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
                mvwprintw(win, 1, WIDTH_WIN2 / 2 - 4, "%s", msg);
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
        mvwprintw(win, 1, WIDTH_WIN2 / 2 - 4, "You died!");
        wrefresh(win);
        sleep(2);
        werase(win);
        wrefresh(win);
        return;
    }
}