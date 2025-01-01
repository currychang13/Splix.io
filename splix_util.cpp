#include "splix_header.h"

ssize_t readline(int fd, char *buffer, size_t maxlen)
{
    size_t n = 0;
    ssize_t bytes_read;
    char c;

    while (n < maxlen - 1)
    {
        bytes_read = read(fd, &c, 1); // Read one character at a time
        if (bytes_read == 1)
        {
            buffer[n++] = c;
            if (c == '\n')
            {
                break; // Stop when newline is encountered
            }
        }
        else if (bytes_read == 0)
        {
            break;
        }
        else
        {
            return -1;
        }
    }
    buffer[n] = '\0';
    return n;
}

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
void Initial_Window::Show_Instruction()
{
    wattron(win, COLOR_PAIR(3) | A_BOLD | A_BLINK);
    mvwprintw(win, 18, (WIDTH_INIT_WIN - 30) / 2, "Type your name and press Enter");
    wattroff(win, COLOR_PAIR(3) | A_BOLD | A_BLINK);
};
void Rule_Window::Show_Rules()
{
    mvwprintw(win, 1, (width - 12) / 2, "How to Play");
    mvwprintw(win, 4, (width - 31) / 2, "Use arrow keys or WASD to move");
    mvwprintw(win, 6, (width - 23) / 2, "Press F to boost speed");
    mvwprintw(win, 8, (width - 21) / 2, "Press G to slow down");
    mvwprintw(win, 10, (width - 25) / 2, "Press Q to quit the game");
    wrefresh(win);
};

// Room_Window functions
void Select_Room_Window::select_room(std::vector<std::pair<int, int>> room_info)
{
    nodelay(win, TRUE);
    keypad(win, TRUE);
    curs_set(0);
    noecho();
    if (room_info.size() == 0)
    {
        mvwprintw(win, 11, (width - 16) / 2, "No room here...");
    }

    // Display all room options and the option to create a new room
    for (int i = 0; i < (int)room_info.size(); i++)
    {
        std::string room_str = "Room " + std::to_string(room_info[i].first) + ": " + std::to_string(room_info[i].second) + " players in the room";
        mvwprintw(win, 2 * i + 13, (width - room_str.length()) / 2, "%s", room_str.c_str());
    }
    mvwprintw(win, 2 * room_info.size() + 13, (width - 18) / 2, "Create a new room");
    mvwprintw(win, 2 * (room_info.size() + 1) + 13, (width - 16) / 2, "Return to lobby");

    bool selected = false;
    selected_room = 0;

    if (room_info.size() > 0)
    {
        wattron(win, A_REVERSE);
        std::string initial_str = "Room " + std::to_string(room_info[0].first) + ": " + std::to_string(room_info[0].second) + " players in the room";
        mvwprintw(win, 13, (width - initial_str.length()) / 2, "%s", initial_str.c_str());
        wattroff(win, A_REVERSE);
        wrefresh(win);
    }
    else
    {
        wattron(win, A_REVERSE);
        mvwprintw(win, 13, (width - 18) / 2, "Create a new room");
        wattroff(win, A_REVERSE);
        wrefresh(win);
    }
    while (!selected)
    {
        int ch = wgetch(win);

        // Remove highlighting from the currently selected room
        if (selected_room < room_info.size())
        {
            std::string current_str = "Room " + std::to_string(room_info[selected_room].first) + ": " + std::to_string(room_info[selected_room].second) + " players in the room";
            mvwprintw(win, 2 * selected_room + 13, (width - current_str.length()) / 2, "%s", current_str.c_str());
        }
        else if (selected_room == room_info.size())
        {
            mvwprintw(win, 2 * room_info.size() + 13, (width - 18) / 2, "Create a new room");
        }
        else
        {
            mvwprintw(win, 2 * (room_info.size() + 1) + 13, (width - 16) / 2, "Return to lobby");
        }

        if (ch == KEY_UP)
        {
            selected_room = (selected_room - 1 >= 0) ? selected_room - 1 : 0;
        }
        else if (ch == KEY_DOWN)
        {
            selected_room = (selected_room + 1 <= (int)room_info.size() + 1) ? selected_room + 1 : room_info.size() + 1;
        }
        else if (ch == '\n')
        {
            selected = true;
        }

        // Highlight the newly selected room
        if (selected_room < room_info.size())
        {
            wattron(win, A_REVERSE);
            std::string new_str = "Room " + std::to_string(room_info[selected_room].first) + ": " + std::to_string(room_info[selected_room].second) + " players in the room";
            mvwprintw(win, 2 * selected_room + 13, (width - new_str.length()) / 2, "%s", new_str.c_str());
            wattroff(win, A_REVERSE);
        }
        else if (selected_room == room_info.size())
        {
            wattron(win, A_REVERSE);
            mvwprintw(win, 2 * room_info.size() + 13, (width - 18) / 2, "Create a new room");
            wattroff(win, A_REVERSE);
        }
        else
        {
            wattron(win, A_REVERSE);
            mvwprintw(win, 2 * (room_info.size() + 1) + 13, (width - 16) / 2, "Return to lobby");
            wattroff(win, A_REVERSE);
        }
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
void Create_Room_Window::Show_Instruction()
{
    wattron(win, COLOR_PAIR(3) | A_BOLD | A_BLINK);
    mvwprintw(win, 18, (WIDTH_INIT_WIN - 30) / 2, "Type RoomID and press Enter");
    wattroff(win, COLOR_PAIR(3) | A_BOLD | A_BLINK);
}
// CR_Input_Window functions
void CR_Input_Window::get_user_input()
{
    noecho();
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
            if (strlen(id) == 0)
            {
                wattron(win, COLOR_PAIR(18) | A_BOLD);
                mvwprintw(win, 1, 1, "Can't enter empty room id");
                wattroff(win, COLOR_PAIR(18) | A_BOLD);
                wrefresh(win);
                warning = 1;
                continue;
            }
            break;
        }
        if (i == 0)
        {
            mvwprintw(win, 1, 1, "                         ");
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
                mvwprintw(win, 1, 1, "                         ");
                mvwprintw(win, 1, 1, "Enter room id");
                wmove(win, 1, 1);
            }
            continue;
        }
        if (ch == KEY_LEFT || ch == KEY_RIGHT || ch == KEY_UP || ch == KEY_DOWN)
        {
            if (warning)
            {
                mvwprintw(win, 1, 1, "                          ");
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
            mvwprintw(win, 1, 1, "                          ");
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

// Room_Window functions
void Room_Window::Render_room()
{
    wattron(win, COLOR_PAIR(1) | A_BOLD);
    int maxX = getmaxx(win);
    setlocale(LC_ALL, "");
    const wchar_t *title[] = {
        L"██████╗ ███████╗ █████╗ ██████╗ ██╗   ██╗██████╗ ",
        L"██╔══██╗██╔════╝██╔══██╗██╔══██╗╚██╗ ██╔╝╚════██╗",
        L"██████╔╝█████╗  ███████║██║  ██║ ╚████╔╝   ▄███╔╝",
        L"██╔══██╗██╔══╝  ██╔══██║██║  ██║  ╚██╔╝    ▀▀══╝ ",
        L"██║  ██║███████╗██║  ██║██████╔╝   ██║     ██╗   ",
        L"╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═════╝    ╚═╝     ╚═╝   ",
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
void Room_Window::inside_room(std::vector<std::string> member_info, int room_id)
{
    nodelay(win, TRUE); // Non-blocking input
    keypad(win, TRUE);
    curs_set(0);

    int num_players = member_info.size();
    int half_players = (num_players + 1) / 2; // First line has half (round up)
    Window Room_member_win(2 * half_players + 10, width / 1.5, 17, (COLS - width / 1.5) / 2);
    int row_start = 1;
    int col_start_first = 3;
    int col_start_second = Room_member_win.width - 26;

    // Display room ID
    std::string room_id_str = "Room " + std::to_string(room_id);
    wattron(Room_member_win.win, A_BOLD);
    mvwprintw(Room_member_win.win, row_start, 2, "%s", room_id_str.c_str());
    wattroff(Room_member_win.win, A_BOLD);
    Room_member_win.draw();
    row_start += 2;

    for (int i = 0; i < num_players; ++i)
    {
        std::string player_str = "Player " + std::to_string(i + 1) + ": " + member_info[i];
        mvwprintw(Room_member_win.win, row_start, (i % 2 == 0) ? col_start_first : col_start_second, "%s", player_str.c_str());
        if (i % 2 == 1)
        {
            row_start += 3;
        }
    }

    int pos_row = Room_member_win.height - 2, pos_col_first = 28, pos_col_second = pos_col_first + 13;

    wattron(Room_member_win.win, A_REVERSE);
    mvwprintw(Room_member_win.win, pos_row, pos_col_first, "Enter Game");
    wattroff(Room_member_win.win, A_REVERSE);
    mvwprintw(Room_member_win.win, pos_row, pos_col_second, "Back to Room Selection");
    wrefresh(Room_member_win.win);

    // Handle user selection
    selected_object = num_players; // Start with "Enter Game" selected
    bool selected = false;

    while (!selected)
    {
        int ch = wgetch(win);
        // Remove highlighting from the currently selected option
        if (selected_object == num_players)
        {
            mvwprintw(Room_member_win.win, pos_row, pos_col_first, "Enter Game");
        }
        else if (selected_object == num_players + 1)
        {
            mvwprintw(Room_member_win.win, pos_row, pos_col_second, "Back to Room Selection");
        }

        // Update selection based on user input
        if (ch == KEY_LEFT)
        {
            selected_object = (selected_object - 1 >= num_players) ? selected_object - 1 : num_players;
        }
        else if (ch == KEY_RIGHT)
        {
            selected_object = (selected_object + 1 <= num_players + 1) ? selected_object + 1 : num_players + 1;
        }
        else if (ch == '\n')
        {
            selected = true; // Confirm selection
        }

        // Highlight the newly selected option
        if (selected_object == num_players)
        {
            wattron(Room_member_win.win, A_REVERSE);
            mvwprintw(Room_member_win.win, pos_row, pos_col_first, "Enter Game");
            wattroff(Room_member_win.win, A_REVERSE);
        }
        else if (selected_object == num_players + 1)
        {
            wattron(Room_member_win.win, A_REVERSE);
            mvwprintw(Room_member_win.win, pos_row, pos_col_second, "Back to Room Selection");
            wattroff(Room_member_win.win, A_REVERSE);
        }
        wrefresh(Room_member_win.win);
    }
}

// Input_Window functions
void Input_Window::get_user_input()
{
    noecho(); // enable displaying input
    keypad(win, TRUE);
    wmove(win, 1, 1);
    int ch, i = 0, cursor_position = 1, warning = 0;
    memset(name, 0, name_length);
    while (true)
    {
        ch = wgetch(win);
        if (ch == '\n')
        {
            if (strlen(name) == 0)
            {
                wattron(win, COLOR_PAIR(18) | A_BOLD);
                mvwprintw(win, 1, 1, "Can't enter empty name");
                wattroff(win, COLOR_PAIR(18) | A_BOLD);
                wrefresh(win);
                warning = 1;
                continue;
            }
            break;
        }
        if (i == 0)
        {
            mvwprintw(win, 1, 1, "                         ");
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
                mvwprintw(win, 1, 1, "                         ");
                mvwprintw(win, 1, 1, "Enter your name");
                wmove(win, 1, 1);
            }
            continue;
        }
        if (ch == KEY_LEFT || ch == KEY_RIGHT || ch == KEY_UP || ch == KEY_DOWN)
        {
            if (warning)
            {
                mvwprintw(win, 1, 1, "                         ");
                mvwprintw(win, 1, 1, "%s", name);
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
                    if (cursor_position <= (int)strlen(name))
                        ++cursor_position;
                    break;
                default:
                    break;
                }
            }
        }
        else if (i < name_length && isprint(ch))
        {
            mvwprintw(win, 1, 1, "                         ");
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
void Splix_Window::render_game(int coordinate_y, int coordinate_x, Mode mode)
{
    int half_rows = height / 2;
    int half_cols = width / 2;

    int start_y = std::max(0, coordinate_y - half_rows);
    int start_x = std::max(0, coordinate_x - half_cols);

    if (start_y + height > MAP_HEIGHT)
        start_y = MAP_HEIGHT - height;
    if (start_x + width > MAP_WIDTH)
        start_x = MAP_WIDTH - width;

    Custom_Blink_Border(win,
                        coordinate_y <= half_rows,
                        coordinate_y >= MAP_HEIGHT - half_rows,
                        coordinate_x <= half_cols,
                        coordinate_x >= MAP_WIDTH - half_cols);

    setlocale(LC_ALL, "");

    int visible_start_y = std::max(0, start_y);
    int visible_start_x = std::max(0, start_x);
    int visible_end_y = std::min(MAP_HEIGHT, start_y + height - 2);
    int visible_end_x = std::min(MAP_WIDTH, start_x + width - 2);

    for (int i = visible_start_y; i < visible_end_y; ++i)
    {
        for (int j = visible_start_x; j < visible_end_x; ++j)
        {
            int map_y = i;
            int map_x = j;

            const wchar_t *symbol = nullptr;
            int value = map[map_y][map_x];
            int color_pair;

            if (value == 0)
            {
                color_pair = 19;
                symbol = L".";
            }
            else if (value < 0)
            {
                color_pair = -value;
                symbol = L"■";
            }
            else if (value > 0)
            {
                color_pair = value;
                if (mode == Mode::FAST)
                    symbol = L"★";
                else
                    symbol = L"▪";
            }
            if (map_y == coordinate_y && map_x == coordinate_x)
            {
                symbol = L"◯";
            }
            wattron(win, COLOR_PAIR(color_pair));
            mvwaddwstr(win, map_y - visible_start_y + 1, map_x - visible_start_x + 1, symbol);
            wattroff(win, COLOR_PAIR(color_pair));
        }
    }
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
    } // send map to server
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
bool Splix_Window::check_valid_position(int coordinate_y, int coordinate_x)
{
    // three cases
    //  1. empty territory or other player's territory
    //  2. territory of the player(-id), fill territory
    //  3. trail of the player(id), die
    //  4. out of bound, die
    // out of bound or touch the trail -> die
    if (coordinate_y < 1 || coordinate_y >= MAP_HEIGHT - 1 || coordinate_x < 1 || coordinate_x >= MAP_WIDTH - 1 || map[coordinate_y][coordinate_x] == id)
    {
        exit_game(0); // die
        return false;
    }
    return true;
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
                draw();
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
        draw();
        mvwprintw(win, 1, WIDTH_GAME_WIN / 2 - 4, "You died!");
        wrefresh(win);
        sleep(2);
        werase(win);
        wrefresh(win);
        return;
    }
}

// Status_Window functions
void Status_Window::update_status(int coordinate_y, int coordinate_x, const char *mode, int id)
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
    mvwprintw(win, 3, 1, "                     ");
    mvwprintw(win, 3, 1, "Position: (%d, %d)", coordinate_y, coordinate_x);
    mvwprintw(win, 4, 1, "                   ");
    mvwprintw(win, 4, 1, "Mode: %s", mode);
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
    wattroff(win, COLOR_PAIR(1) | A_BOLD);
    wrefresh(win);
}

// udp functions
void UdpContent::udp_connect()
{
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    // Initialize server address
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr);
    if (connect(sockfd, (const sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("Connect failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    napms(1);
    send(sockfd, "ack", 3, MSG_CONFIRM);
    // sendto(sockfd, "ack", 3, MSG_CONFIRM, (const sockaddr *)&servaddr, sizeof(servaddr));
}
void UdpContent::send_server_position(int coordinate_y, int coordinate_x, int id, Mode mode)
{
    char message[BUFFER_SIZE] = "";
    sprintf(message, "%d %d %s", coordinate_y, coordinate_x, mode == Mode::FAST ? "FAST" : "NORMAL"); // send position, mode to server
    send(sockfd, message, strlen(message), MSG_CONFIRM);
}
int UdpContent::get_id_from_server()
{
    char message[BUFFER_SIZE];
    int n;
    n = recv(sockfd, message, BUFFER_SIZE, 0);
    message[n] = '\0';
    return atoi(message);
}
std::pair<int, int> UdpContent::get_position_from_server()
{
    char message[BUFFER_SIZE];
    int n;
    n = recv(sockfd, message, BUFFER_SIZE, 0);
    message[n] = '\0';
    std::pair<int, int> position;
    sscanf(message, "%d %d", &position.first, &position.second);
    return position;
}
void UdpContent::send_leave_game()
{
    char message[BUFFER_SIZE] = "leave";
    send(sockfd, message, strlen(message), MSG_CONFIRM);
}


// tcp functions
void TcpContent::tcp_connect()
{
    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int n;
    // Initialize server address
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr);

    // Connect to server
    if ((n = connect(sockfd, (const sockaddr *)&servaddr, sizeof(servaddr))) < 0)
    {
        perror("connection failed");
    };
}
void TcpContent::send_server_name(char *name)
{
    int n;
    if ((n = write(sockfd, name, strlen(name))) < 0)
    {
        perror("write");
        exit(1);
    }
}
void TcpContent::send_server_room_id(int room_id)
{
    char room_str[100] = "";
    sprintf(room_str, "%d", room_id);
    room_str[strlen(room_str)] = '\0';
    write(sockfd, room_str, strlen(room_str));
}
void TcpContent::receive_room_info(std::vector<std::pair<int, int>> &room_info)
{
    // Receive room info from server
    char room_number[100] = "";
    char room_str[100] = "";
    readline(sockfd, room_number, sizeof(room_number));
    int room_num = atoi(room_number);
    for (int i = 0; i < room_num; i++)
    {
        int room_id, room_player;
        readline(sockfd, room_str, sizeof(room_str));
        sscanf(room_str, "%d %d", &room_id, &room_player);
        room_info.push_back({room_id, room_player});
    }
}
void TcpContent::receive_member_info(std::vector<std::string> &member_info)
{
    member_info.clear();
    // receive member info from server
    char member_number[10] = "";
    char member_str[20] = "";
    readline(sockfd, member_number, sizeof(member_number));
    int member_num = atoi(member_number);
    for (int i = 0; i < member_num; i++)
    {
        int n = readline(sockfd, member_str, sizeof(member_str));
        member_str[n - 1] = '\0';
        member_info.push_back(member_str);
    }
}
void TcpContent::send_return_to_room_selection()
{
    char message[100] = "";
    sprintf(message, "Return to Room Selection");
    write(sockfd, message, strlen(message));
}
void TcpContent::send_start()
{
    char message[100] = "";
    sprintf(message, "start");
    write(sockfd, message, strlen(message));
}
void TcpContent::receive_port(int &port)
{
    char port_str[100] = "";
    readline(sockfd, port_str, sizeof(port_str));
    port = atoi(port_str);
}

// player functions
void Player::init(std::pair<int, int> position, std::pair<int, int> direction, int id, Mode mode, int acceleration_timer, int cooldown_timer, int score)
{
    coordinate_x = position.second;
    coordinate_y = position.first;
    this->direction = direction;
    this->id = id;
    this->mode = mode;
    this->acceleration_timer = acceleration_timer;
    this->cooldown_timer = cooldown_timer;
    this->score = score;
}
