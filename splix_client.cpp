#include "splix_header.h"
int map[MAP_HEIGHT][MAP_WIDTH];
enum class GameStatus
{
    INITIAL,
    ROOM_SELECTION,
    GAMING,
    GAME_OVER
};

void game_loop(Splix_Window *game_win, Status_Window *stat_win)
{
    // Disable the cursor
    curs_set(0);
    keypad(game_win->win, TRUE);
    nodelay(game_win->win, TRUE); // Non-blocking input
    wattron(game_win->win, COLOR_PAIR(1) | A_BOLD);
    game_win->draw();
    stat_win->draw();
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
    game_win->create_initial_territory(coordinate_y, coordinate_x);
    game_win->render_game(coordinate_y, coordinate_x);
    stat_win->update_status(coordinate_y, coordinate_x);
    // gaming
    int ch; // use int to store the character like key_up, key_down, etc.
    for (;;)
    {
        if ((ch = wgetch(game_win->win)) != ERR)
        {
            std::pair<int, int> new_direction = direction;
            switch (ch)
            {
            case 'q':
                wattroff(game_win->win, COLOR_PAIR(1) | A_BOLD);
                game_win->exit_game(1);
                return; // exit the function
            case 'w':
            case KEY_UP:
                new_direction = {-1, 0};
                break; // break means exit the switch, not the loop,which ignores the operation
            case 's':
            case KEY_DOWN:
                new_direction = {1, 0};
                break;
            case 'a':
            case KEY_LEFT:
                new_direction = {0, -1};
                break;
            case 'd':
            case KEY_RIGHT:
                new_direction = {0, 1};
                break;
            default:
                break;
            }
            if (new_direction.first != -direction.first && new_direction.second != -direction.second)
            {
                direction = new_direction;
            }
        }

        // update position
        coordinate_y += direction.first;
        coordinate_x += direction.second;

        // check if die or not
        if (!game_win->check_valid_position(coordinate_y, coordinate_x))
            return;
        // check the territory
        if (map[coordinate_y][coordinate_x] == -id)
        {
            // if circled, else, moving on its own territory
            if (game_win->is_enclosure(coordinate_y, coordinate_x))
            {
                // Find all inside points
                auto inside_points = game_win->find_inside_points();

                // Fill the enclosed area
                game_win->fill_territory(inside_points);
            }
        }
        else
        {
            map[coordinate_y][coordinate_x] = id;
        }
        game_win->render_game(coordinate_y, coordinate_x);
        stat_win->update_status(coordinate_y, coordinate_x);
        usleep(200000); // Sleep for 0.1sec, speed of the game
    }
}

int connect_to_server()
{
    int sockfd;
    struct sockaddr_in servaddr;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Initialize server address
    bzero(&servaddr, sizeof(servaddr));                       // Clear struct
    servaddr.sin_family = AF_INET;                            // IPv4
    servaddr.sin_port = 12345;                                // Server port (convert to network byte order)
    inet_pton(AF_INET, "140.113.66.205", &servaddr.sin_addr); // Convert IP address to binary form

    // Connect to server
    connect(sockfd, (const sockaddr *)&servaddr, sizeof(servaddr));

    return sockfd;
}
void send_server_name(int sockfd, const char *name)
{
    // Send name to server
    write(sockfd, (void *)name, strlen(name));
}
std::vector<std::pair<int, int>> receive_room_info(int sockfd)
{
    // Receive room info from server
    char room_number[100];
    char room_str[100];
    std::vector<std::pair<int, int>> room_info;
    read(sockfd, room_number, sizeof(room_number));
    int room_num = atoi(room_number);
    for (int i = 0; i < room_num; i++)
    {
        int room_id, room_player;
        read(sockfd, room_str, sizeof(room_str));
        sscanf(room_str, "%d %d", &room_id, &room_player);
        room_info.push_back({room_id, room_player});
    }
    return room_info;
}
void send_server_room(int sockfd, int room_id)
{
    // Send room id to server
    char room_str[10];
    sprintf(room_str, "%d", room_id);
    write(sockfd, (void *)room_str, strlen(room_str));
}
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
    GameStatus status = GameStatus::INITIAL;
    // windows
    while (true)
    {
        Initial_Window init_win(HEIGHT_INIT_WIN, WIDTH_INIT_WIN, (LINES - HEIGHT_INIT_WIN) / 2, (COLS - WIDTH_INIT_WIN) / 2);
        Room_Window room_win(HEIGHT_INIT_WIN, WIDTH_INIT_WIN, (LINES - HEIGHT_INIT_WIN) / 2, (COLS - WIDTH_INIT_WIN) / 2);
        Input_Window input_win(HEIGHT_INIT_WIN / 12, WIDTH_GAME_WIN / 2, (HEIGHT_INIT_WIN - HEIGHT_GAME_WIN / 12) / 2.5, (COLS - WIDTH_GAME_WIN / 2) / 2);
        Status_Window stat_win(HEIGHT_GAME_WIN / 10, WIDTH_GAME_WIN / 4, (LINES - HEIGHT_GAME_WIN) / 2, (COLS + WIDTH_GAME_WIN) / 2);
        Splix_Window splix_win(HEIGHT_GAME_WIN, WIDTH_GAME_WIN, (LINES - HEIGHT_GAME_WIN) / 2, (COLS - WIDTH_GAME_WIN) / 2);

        // int sockfd = connect_to_server();
        std::vector<std::pair<int, int>> room_info;
        room_info.push_back({1, 2});
        room_info.push_back({2, 3});
        switch (status)
        {
        case GameStatus::INITIAL:
            init_win.draw();
            init_win.Rendertitle();
            input_win.draw();
            input_win.get_user_input();
            // send name to server
            // send_server_name(sockfd, input_win.name);
            // room_info = receive_room_info(sockfd);
            status = GameStatus::ROOM_SELECTION;
            break;
        case GameStatus::ROOM_SELECTION:
            room_win.draw();
            room_win.Renderroom();
            room_win.select_room(room_info);
            if (room_win.quit)
            {
                status = GameStatus::INITIAL;
                break;
            }
            //send_server_room(sockfd, room_info[room_win.selected_room].first);
            status = GameStatus::GAMING;
            break;
        case GameStatus::GAMING:
            noecho(); // disable displaying input
            game_loop(&splix_win, &stat_win);
            echo(); // enable displaying input
            status = GameStatus::GAME_OVER;
            break;
        case GameStatus::GAME_OVER:
            // show game over
            mvwprintw(stdscr, 0, 0, "Game Over. Press 'r' to restart or 'q' to quit.");
            refresh();

            int ch = getch();
            if (ch == 'q')
            {
                endwin(); // Exit the loop, ending the program
            }
            else if (ch != 'r')
            {
                status = GameStatus::INITIAL; // Default behavior is to exit unless 'r' is pressed
            }
            break;
        }
        // after game over
    }
    return 0;
}
