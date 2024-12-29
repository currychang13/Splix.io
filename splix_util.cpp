#include "splix_header.h"
// Initial_Window functions
void Initial_Window::Rendertitle()
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
        L"╚══════╝╚═╝     ╚══════╝╚═╝╚═╝  ╚═╝╚═╝╚═╝ ╚═════╝",
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

// Room_Window functions
void Select_Room_Window::select_room(std::vector<std::pair<int, int>> room_info)
{
    nodelay(win, TRUE); // Non-blocking input
    keypad(win, TRUE);

    // Get the dimensions of the window
    if (room_info.size() == 0)
    {
        mvwprintw(win, 13, (width - 16) / 2, "No room here...");
    }

    // Display all room options and the option to create a new room
    for (int i = 0; i < (int)room_info.size(); i++)
    {
        std::string room_str = "Room ID " + std::to_string(room_info[i].first) + ": " + std::to_string(room_info[i].second) + " players in the room";
        mvwprintw(win, 2 * i + 15, (width - room_str.length()) / 2, "%s", room_str.c_str());
    }
    mvwprintw(win, 2 * room_info.size() + 15, (width - 18) / 2, "Create a new room");
    mvwprintw(win, 2 * (room_info.size() + 1) + 15, (width - 16) / 2, "Return to lobby");
    // quit message
    bool selected = false;
    selected_room = 0;

    // Highlight the initial selection
    if (room_info.size() > 0)
    {
        wattron(win, A_REVERSE);
        std::string initial_str = "Room ID " + std::to_string(room_info[0].first) + ": " + std::to_string(room_info[0].second) + " players in the room";
        mvwprintw(win, 15, (width - initial_str.length()) / 2, "%s", initial_str.c_str());
        wattroff(win, A_REVERSE);
        wrefresh(win);
    }
    else
    {
        wattron(win, A_REVERSE);
        mvwprintw(win, 15, (width - 16) / 2, "Create a new room");
        wattroff(win, A_REVERSE);
        wrefresh(win);
    }
    while (!selected)
    {
        int ch = wgetch(win);

        // Remove highlighting from the currently selected room
        if (selected_room < room_info.size())
        {
            std::string current_str = "Room ID " + std::to_string(room_info[selected_room].first) + ": " + std::to_string(room_info[selected_room].second) + " players in the room";
            mvwprintw(win, 2 * selected_room + 15, (width - current_str.length()) / 2, "%s", current_str.c_str());
        }
        else if (selected_room == room_info.size())
        {
            mvwprintw(win, 2 * room_info.size() + 15, (width - 18) / 2, "Create a new room");
        }
        else
        {
            mvwprintw(win, 2 * (room_info.size() + 1) + 15, (width - 16) / 2, "Return to lobby");
        }

        if (ch == KEY_UP)
        {
            // Move selection up
            selected_room = (selected_room - 1 >= 0) ? selected_room - 1 : 0;
        }
        else if (ch == KEY_DOWN)
        {
            // Move selection down
            selected_room = (selected_room + 1 <= (int)room_info.size() + 1) ? selected_room + 1 : room_info.size() + 1;
        }
        else if (ch == '\n')
        {
            // Confirm selection
            selected = true;
        }
        // Highlight the newly selected room
        if (selected_room < room_info.size())
        {
            wattron(win, A_REVERSE);
            std::string new_str = "Room ID " + std::to_string(room_info[selected_room].first) + ": " + std::to_string(room_info[selected_room].second) + " players in the room";
            mvwprintw(win, 2 * selected_room + 15, (width - new_str.length()) / 2, "%s", new_str.c_str());
            wattroff(win, A_REVERSE);
        }
        else if (selected_room == room_info.size())
        {
            wattron(win, A_REVERSE);
            mvwprintw(win, 2 * room_info.size() + 15, (width - 18) / 2, "Create a new room");
            wattroff(win, A_REVERSE);
        }
        else
        {
            wattron(win, A_REVERSE);
            mvwprintw(win, 2 * (room_info.size() + 1) + 15, (width - 16) / 2, "Return to lobby");
            wattroff(win, A_REVERSE);
        }
        // Refresh the window to display changes
        wrefresh(win);
    }
}
void Select_Room_Window::Render_select_room()
{
    wattron(win, COLOR_PAIR(1) | A_BOLD);
    int maxX = getmaxx(win);
    setlocale(LC_ALL, "");
    const wchar_t *title[] = {
        L"███████╗███████╗██╗     ███████╗ ██████╗████████╗    ██████╗  ██████╗  ██████╗ ███╗   ███╗███████╗",
        L"██╔════╝██╔════╝██║     ██╔════╝██╔════╝╚══██╔══╝    ██╔══██╗██╔═══██╗██╔═══██╗████╗ ████║██╔════╝",
        L"███████╗█████╗  ██║     █████╗  ██║        ██║       ██████╔╝██║   ██║██║   ██║██╔████╔██║███████",
        L"╚════██║██╔══╝  ██║     ██╔══╝  ██║        ██║       ██╔══██╗██║   ██║██║   ██║██║╚██╔╝██║╚════██║",
        L"███████║███████╗███████╗███████╗╚██████╗   ██║       ██║  ██║╚██████╔╝╚██████╔╝██║ ╚═╝ ██║███████║",
        L"╚══════╝╚══════╝╚══════╝╚══════╝ ╚═════╝   ╚═╝       ╚═╝  ╚═╝ ╚═════╝  ╚═════╝ ╚═╝     ╚═╝╚══════╝",
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

// Create_Room_Window functions
void Create_Room_Window::Render_create_room()
{
    wattron(win, COLOR_PAIR(1) | A_BOLD);
    int maxX = getmaxx(win);
    setlocale(LC_ALL, "");
    const wchar_t *title[] = {
        L" ██████╗██████╗ ███████╗ █████╗ ████████╗███████╗    ██████╗  ██████╗  ██████╗ ███╗   ███╗███████╗",
        L"██╔════╝██╔══██╗██╔════╝██╔══██╗╚══██╔══╝██╔════╝    ██╔══██╗██╔═══██╗██╔═══██╗████╗ ████║██╔════╝",
        L"██║     ██████╔╝█████╗  ███████║   ██║   █████╗      ██████╔╝██║   ██║██║   ██║██╔████╔██║███████╗",
        L"██║     ██╔══██╗██╔══╝  ██╔══██║   ██║   ██╔══╝      ██╔══██╗██║   ██║██║   ██║██║╚██╔╝██║╚════██║",
        L"╚██████╗██║  ██║███████╗██║  ██║   ██║   ███████╗    ██║  ██║╚██████╔╝╚██████╔╝██║ ╚═╝ ██║███████║",
        L" ╚═════╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝   ╚═╝   ╚══════╝    ╚═╝  ╚═╝ ╚═════╝  ╚═════╝ ╚═╝     ╚═╝╚══════╝",
    };
    int artWidth = wcslen(title[0]);
    int startX = (maxX - artWidth) / 2;
    int artLines = sizeof(title) / sizeof(title[0]);
    int startY = 3;
    for (int i = 0; i < artLines; i++)
        mvwaddwstr(win, startY + i, startX, title[i]);
    wrefresh(win);
    wattroff(win, COLOR_PAIR(1) | A_BOLD);
};

// CR_Input_Window functions
void CR_Input_Window::get_user_input()
{
    noecho(); // enable displaying input
    keypad(win, TRUE);
    wmove(win, 1, 1);
    int ch, i = 0, cursor_position = 1;
    int warning = 0;
    memset(id, 0, id_length);
    while (true)
    {
        ch = wgetch(win);
        if (ch == '\n')
        {
            break;
        }
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
                    id[j - 1] = id[j];
                }

                i--;          // Reduce string length
                id[i] = '\0'; // Null-terminate the string

                cursor_position--;               // Move cursor position back
                mvwprintw(win, 1, 1, "%s ", id); // Redraw string and clear last char
                wmove(win, 1, cursor_position);  // Set cursor position
            }

            if (cursor_position <= 1)
            {
                mvwprintw(win, 1, 1, "Enter room id");
                wmove(win, 1, 1);
            }
            continue;
        }
        if (ch == KEY_LEFT || ch == KEY_RIGHT || ch == KEY_UP || ch == KEY_DOWN)
        {
            if (warning)
            {
                mvwprintw(win, 1, 1, "                     ");
                mvwprintw(win, 1, 1, "%s", id);
                warning = 0;
            }
            else
            {
                switch (ch)
                {
                case KEY_LEFT:
                    if (cursor_position > 1)
                        --cursor_position;
                    break;
                case KEY_RIGHT:
                    if (cursor_position <= (int)strlen(id))
                        ++cursor_position;
                    break;
                default:
                    break;
                }
            }
        }
        else if (isdigit(ch) == 0 && i < id_length)
        {
            wattron(win, COLOR_PAIR(18) | A_BOLD);
            mvwprintw(win, 1, 1, "Please enter a number");
            wattroff(win, COLOR_PAIR(18) | A_BOLD);
            wrefresh(win);
            warning = 1;
            continue;
        }
        else if (i < id_length && isdigit(ch))
        {
            mvwprintw(win, 1, 1, "                      ");
            i++; // Increase string length
            // Insert the character at the cursor position
            for (int j = i; j >= cursor_position - 1; j--)
            {
                id[j + 1] = id[j];
            }
            id[cursor_position - 1] = ch;

            id[i] = '\0';                   // Null-terminate the string
            cursor_position++;              // Move cursor position forward
            mvwprintw(win, 1, 1, "%s", id); // Redraw string
            wrefresh(win);
        }

        wmove(win, 1, cursor_position); // Set cursor position
    }
}

// Input_Window functions
void Input_Window::get_user_input()
{
    noecho(); // enable displaying input
    keypad(win, TRUE);
    wmove(win, 1, 1);
    int ch, i = 0, cursor_position = 1;
    memset(name, 0, name_length);
    while (true)
    {
        ch = wgetch(win);
        if (ch == '\n')
        {
            break;
        }
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
            }

            if (cursor_position <= 1)
            {
                mvwprintw(win, 1, 1, "Enter your name");
                wmove(win, 1, 1);
            }
            continue;
        }
        if (ch == KEY_LEFT || ch == KEY_RIGHT || ch == KEY_UP || ch == KEY_DOWN)
        {
            switch (ch)
            {
            case KEY_LEFT:
                if (cursor_position > 1)
                    --cursor_position;
                break;
            case KEY_RIGHT:
                if (cursor_position <= (int)strlen(name))
                    ++cursor_position;
                break;
            default:
                break;
            }
        }
        else if (i < name_length && isprint(ch))
        {
            i++; // Increase string length
            // Insert the character at the cursor position
            for (int j = i; j >= cursor_position - 1; j--)
            {
                name[j + 1] = name[j];
            }
            name[cursor_position - 1] = ch;

            name[i] = '\0';                   // Null-terminate the string
            cursor_position++;                // Move cursor position forward
            mvwprintw(win, 1, 1, "%s", name); // Redraw string
            wrefresh(win);
        }
        wmove(win, 1, cursor_position); // Set cursor position
    }
}

// Splix_Window functions
void Splix_Window::render_game(int coordinate_y, int coordinate_x)
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

    // Change border color if the character is near the map's edge
    bool near_edge = (coordinate_y <= half_rows || coordinate_y >= MAP_HEIGHT - half_rows ||
                      coordinate_x <= half_cols || coordinate_x >= MAP_WIDTH - half_cols);

    // Set border color
    if (near_edge)
    {
        wattron(win, A_BLINK); // Red for edge proximity
        box(win, 0, 0);
        wattroff(win, A_BLINK); // Turn off border color
        wrefresh(win);
    }

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
            if (map[map_y][map_x] == 0)
            {
                symbol = L".";                // Empty space
                wattron(win, COLOR_PAIR(19)); // Gray
            }
            else if (map[map_y][map_x] == -id)
            {
                symbol = L"■"; // Filled territory
                wattron(win, COLOR_PAIR(id));
            }
            else if (map[map_y][map_x] == id)
            {
                if (mode == Mode::FAST)
                    symbol = L"★"; // Player trail in FAST mode
                else
                    symbol = L"▪"; // Player trail in NORMAL mode
                wattron(win, COLOR_PAIR(id));
            }
            else
            {
                symbol = L"?"; // Undefined
                wattron(win, COLOR_PAIR(3));
            }

            // Render the character or cell
            if (map_y == coordinate_y && map_x == coordinate_x)
            {
                symbol = L"◯"; // Player
            }
            mvwaddwstr(win, i + 1, j + 1, symbol);
            wattroff(win, COLOR_PAIR(3) | COLOR_PAIR(id));
        }
    }
    // Refresh the window after rendering
    wrefresh(win);
}
void Splix_Window::create_initial_territory(int coordinate_y, int coordinate_x)
{
    // create initial territory
    // 1. create a rectangle
    // 2. fill the rectangle with the player's -id
    // 3. send the map to the server
    int dx[] = {0, 1, -1, 2, -2};
    int dy[] = {1, 0, -1, 2, -2};
    for (int i = 0; i < 5; i++)
    {
        int cur_x = coordinate_x + dx[i];
        for (int j = 0; j < 5; j++)
        {
            int cur_y = coordinate_y + dy[j];
            if (map[cur_y][cur_x] == -id || cur_x < 0 || cur_x >= MAP_WIDTH || cur_y < 0 || cur_y >= MAP_HEIGHT)
                continue;
            map[cur_y][cur_x] = -id;
        }
    }
    render_game(coordinate_y, coordinate_x);
    // send map to server
}
bool Splix_Window::is_enclosure(int y, int x)
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
std::vector<std::pair<int, int>> Splix_Window::find_inside_points()
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
void Splix_Window::fill_territory(const std::vector<std::pair<int, int>> &inside_points)
{
    for (const auto &[y, x] : inside_points)
    {
        map[y][x] = -id; // Mark as filled territory
    }
}
int Splix_Window::check_valid_position(int coordinate_y, int coordinate_x)
{
    // three cases
    //  1. empty territory or other player's territory
    //  2. territory of the player(-id), fill territory
    //  3. trail of the player(id), die
    //  4. out of bound, die
    // out of bound or touch the trail -> die
    if (coordinate_y < 1 || coordinate_y >= MAP_HEIGHT || coordinate_x < 1 || coordinate_x >= MAP_WIDTH || map[coordinate_y][coordinate_x] == id)
    {
        exit_game(0); // die
        return 0;
    }
    return 1;
}
void Splix_Window::exit_game(int flag)
{
    // send to server
    for (int i = 0; i < HEIGHT_GAME_WIN; i++)
    {
        for (int j = 0; j < WIDTH_GAME_WIN; j++)
        {
            if (map[i][j] == -id)
            {
                map[i][j] = 0;
            }
        }
    }
    // animation
    if (flag == 1) // NORMAL exit
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
                mvwprintw(win, 1, WIDTH_GAME_WIN / 2 - 4, "%s", msg);
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
        mvwprintw(win, 1, WIDTH_GAME_WIN / 2 - 4, "You died!");
        wrefresh(win);
        sleep(2);
        werase(win);
        wrefresh(win);
        return;
    }
}

// Status_Window functions
void Status_Window::update_status(int coordinate_y, int coordinate_x, const char *mode)
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
    mvwprintw(win, 4, 1, "                   ");
    mvwprintw(win, 4, 1, "Mode: %s", mode);
    wrefresh(win);
    wattroff(win, A_BOLD);
}
void Status_Window::update_timer(int acceleration_timer, int cooldown_timer)
{
    setlocale(LC_ALL, "");

    const char *full_block = "=";
    const int bar_length = width - 4;

    int filled_blocks = 0;

    if (acceleration_timer > 0)
    {
        wattron(win, COLOR_PAIR(3) | A_BOLD);
        mvwprintw(win, 6, (width - 10) / 2, "<Boosting>");
        filled_blocks = (acceleration_timer * bar_length) / acc_time;
    }
    else if (cooldown_timer > 0)
    {
        wattron(win, A_BOLD);
        mvwprintw(win, 6, (width - 10) / 2, "<Cooldown>");
        filled_blocks = ((cool_time - cooldown_timer) * bar_length) / cool_time;
    }

    mvwprintw(win, 7, 1, "[");

    for (int i = 0; i < bar_length; i++)
    {
        if (i < filled_blocks)
        {
            mvwprintw(win, 7, 2 + i, "%s", full_block);
        }
        else
        {
            mvwprintw(win, 7, 2 + i, " ");
        }
    }

    mvwprintw(win, 7, width - 2, "]");
    wattroff(win, COLOR_PAIR(3) | A_BOLD);

    wrefresh(win);
}

// Gameover_Window functions
void Gameover_Window::render_gameover()
{
    wattron(win, COLOR_PAIR(1) | A_BOLD);
    int maxX = getmaxx(win);
    setlocale(LC_ALL, "");
    const wchar_t *title[] = {
        L"  ▄████  ▄▄▄       ███▄ ▄███▓▓█████     ▒█████   ██▒   █▓▓█████  ██▀███  ",
        L" ██▒ ▀█▒▒████▄    ▓██▒▀█▀ ██▒▓█   ▀    ▒██▒  ██▒▓██░   █▒▓█   ▀ ▓██ ▒ ██▒",
        L"▒██░▄▄▄░▒██  ▀█▄  ▓██    ▓██░▒███      ▒██░  ██▒ ▓██  █▒░▒███   ▓██ ░▄█ ",
        L"░▓█  ██▓░██▄▄▄▄██ ▒██    ▒██ ▒▓█  ▄    ▒██   ██░  ▒██ █░░▒▓█  ▄ ▒██▀▀█▄ ",
        L"░▒▓███▀▒ ▓█   ▓██▒▒██▒   ░██▒░▒████▒   ░ ████▓▒░   ▒▀█░  ░▒████▒░██▓ ▒██▒",
        L"░▒   ▒  ▒▒   ▓▒█░░ ▒░   ░  ░░░ ▒░ ░   ░ ▒░▒░▒░    ░ ▐░  ░░ ▒░ ░░ ▒▓ ░▒▓░",
        L" ░   ░   ▒   ▒▒ ░░  ░      ░ ░ ░  ░     ░ ▒ ▒░    ░ ░░   ░ ░  ░  ░ ░▒ ░ ▒░",
        L"░ ░   ░  ░   ▒   ░      ░      ░      ░ ░ ░ ▒      ░░     ░     ░ ░░   ░ ",
        L"      ░      ░  ░       ░      ░  ░       ░ ░       ░     ░  ░    ░     ░ ",
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