#include "splix_header.h"

#define DEBUG

// colors
#define COLOR_CORAL 15
#define COLOR_PURPLE 16
#define COLOR_TEAL 17
#define COLOR_DEEPGRAY 18
#define COLOR_GRAY 19

int map[MAP_HEIGHT][MAP_WIDTH];
Mode mode = Mode::NORMAL;
int id = rand() % 11;

int game_loop(Splix_Window *game_win, Status_Window *stat_win)
{
    // Disable the cursor
    curs_set(0);
    keypad(game_win->win, TRUE);
    nodelay(game_win->win, TRUE); // Non-blocking input
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
    std::pair<int, int> direction = {0, 1}; // y and x

    // create initial territory
    game_win->create_initial_territory(coordinate_y, coordinate_x);
    game_win->render_game(coordinate_y, coordinate_x);
    stat_win->update_status(coordinate_y, coordinate_x, "NORMAL");

    // gaming
    int ch; // use int to store the character like key_up, key_down, etc.
    mode = Mode::NORMAL;
    useconds_t frame_time = speed; // 0.15 sec
    int acceleration_timer = acc_time;
    int cooldown_timer = 0;
    for (;;)
    {

        if ((ch = wgetch(game_win->win)) != ERR)
        {
            flushinp(); // clear the input buffer
            std::pair<int, int> new_direction = direction;
            switch (ch)
            {
            case 'q':
                wattroff(game_win->win, COLOR_PAIR(1) | A_BOLD);
                game_win->exit_game(1);
                return 0; // exit the function
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
            case 'f':
                if (mode == Mode::FAST)
                {
                    mode = Mode::NORMAL; // Resume game
                    frame_time = speed;  // Reset to NORMAL speed
                }
                else if (cooldown_timer == 0)
                {
                    mode = Mode::FAST;      // FAST mode
                    frame_time = speed / 3; // FASTer speed: 0.05 seconds
                    acceleration_timer = acc_time;
                }
                break;
            default:
                break;
            }
            if (new_direction.first != -direction.first && new_direction.second != -direction.second)
            {
                direction = new_direction;
            }
        }
        // Handle acceleration timer
        if (mode == Mode::FAST && acceleration_timer > 0)
        {
            acceleration_timer--; // Decrement timer
            if (acceleration_timer == 0)
            {
                mode = Mode::NORMAL;
                frame_time = speed;
                cooldown_timer = cool_time;
            }
        }
        if (mode == Mode::NORMAL && acceleration_timer > 0)
        {
            acceleration_timer++;
        }
        else if (cooldown_timer > 0)
        {
            cooldown_timer--; // Decrement cooldown timer
            if (cooldown_timer == 0)
            {
                acceleration_timer = acc_time;
            }
        }

        coordinate_y += direction.first;
        coordinate_x += direction.second;

        // check if die or not
        if (!game_win->check_valid_position(coordinate_y, coordinate_x))
            return 1;

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
        stat_win->update_status(coordinate_y, coordinate_x, mode == Mode::FAST ? "Burst" : "NORMAL");
        stat_win->update_timer(acceleration_timer, cooldown_timer);
        usleep(frame_time); // Sleep for 0.1sec, speed of the game
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
    servaddr.sin_port = htons(12345);                         // Server port (convert to network byte order)
    inet_pton(AF_INET, "140.113.66.205", &servaddr.sin_addr); // Convert IP address to binary form

    // Connect to server
    connect(sockfd, (const sockaddr *)&servaddr, sizeof(servaddr));
    return sockfd;
}
void send_server_name(int sockfd, const char *name)
{
    //add some error handling
    write(sockfd, name, strlen(name));
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
    write(sockfd, room_str, strlen(room_str));
}
int main()
{
    // signal(SIGWINCH, SIG_IGN); /* ignore window size changes */

    setlocale(LC_ALL, "");
    initscr();
    start_color();
    use_default_colors();
    cbreak();             // disable line buffering, but allow signals(ctrl+c, ctrl+z, etc.)
    keypad(stdscr, TRUE); // enable function keys, arrow keys, etc. stdscr is the default window
    init_color(COLOR_GRAY, 500, 500, 500);
    //init_color(COLOR_PURPLE, 800, 400, 900);   // Light Purple (RGB: 80%, 40%, 90%)
    init_color(COLOR_TEAL, 200, 700, 700);     // Light Teal (RGB: 0%, 50%, 50%)
    init_color(COLOR_CORAL, 1000, 500, 400);   // Coral
    init_color(COLOR_DEEPGRAY, 300, 300, 300); // Dark Gray

    init_pair(0, COLOR_BLACK, -1);
    init_pair(1, COLOR_RED, -1);
    init_pair(2, COLOR_GREEN, -1);
    init_pair(3, COLOR_YELLOW, -1);
    init_pair(4, COLOR_BLUE, -1);
    init_pair(5, COLOR_MAGENTA, -1);
    init_pair(6, COLOR_CYAN, -1);
    init_pair(7, COLOR_WHITE, -1);
    // custom colors
    init_pair(8, COLOR_CORAL, -1);
    init_pair(9, COLOR_PURPLE, -1);
    init_pair(10, COLOR_TEAL, -1);

    // preserved color
    init_pair(18, COLOR_RED, COLOR_WHITE);
    init_pair(19, COLOR_GRAY, -1);
    init_pair(20, COLOR_BLACK, COLOR_WHITE);
    GameStatus status = GameStatus::INITIAL;

#ifndef DEBUG
    int sockfd = connect_to_server();
#endif
    // windows
    Initial_Window init_win(HEIGHT_INIT_WIN, WIDTH_INIT_WIN, (LINES - HEIGHT_INIT_WIN) / 2, (COLS - WIDTH_INIT_WIN) / 2);
    Select_Room_Window select_room_win(HEIGHT_INIT_WIN, WIDTH_INIT_WIN, (LINES - HEIGHT_INIT_WIN) / 2, (COLS - WIDTH_INIT_WIN) / 2);
    Create_Room_Window create_win(HEIGHT_INIT_WIN, WIDTH_INIT_WIN, (LINES - HEIGHT_INIT_WIN) / 2, (COLS - WIDTH_INIT_WIN) / 2);
    CR_Input_Window cr_input_win(HEIGHT_INIT_WIN / 13, WIDTH_INIT_WIN / 2, (HEIGHT_INIT_WIN - HEIGHT_INIT_WIN / 12) / 2.5, (COLS - WIDTH_INIT_WIN / 2) / 2);
    Room_Window room_win(HEIGHT_INIT_WIN, WIDTH_INIT_WIN, (LINES - HEIGHT_INIT_WIN) / 2, (COLS - WIDTH_INIT_WIN) / 2);
    Input_Window input_win(HEIGHT_INIT_WIN / 13, WIDTH_GAME_WIN / 2, (HEIGHT_INIT_WIN - HEIGHT_GAME_WIN / 12) / 2.5, (COLS - WIDTH_GAME_WIN / 2) / 2);
    Status_Window stat_win(HEIGHT_GAME_WIN / 5, WIDTH_GAME_WIN / 4, (LINES - HEIGHT_GAME_WIN) / 2, (COLS + WIDTH_GAME_WIN) / 2);
    Splix_Window splix_win(HEIGHT_GAME_WIN, WIDTH_GAME_WIN, (LINES - HEIGHT_GAME_WIN) / 2, (COLS - WIDTH_GAME_WIN) / 2);
    Gameover_Window gameover_win(HEIGHT_GAME_WIN, WIDTH_GAME_WIN, (LINES - HEIGHT_GAME_WIN) / 2, (COLS - WIDTH_GAME_WIN) / 2);

    while (true)
    {

        std::vector<std::pair<int, int>> room_info;

#ifdef DEBUG
        room_info.push_back({1, 2});
        room_info.push_back({2, 3});
#endif
        switch (status)
        {
        case GameStatus::INITIAL:
            init_win.draw();
            init_win.Rendertitle();
            input_win.draw();
            input_win.get_user_input();
#ifndef DEBUG
            send_server_name(sockfd, input_win.name);
            room_info = receive_room_info(sockfd);
#endif
            status = GameStatus::ROOM_SELECTION;
            break;
        case GameStatus::ROOM_SELECTION:
            select_room_win.draw();
            select_room_win.Render_select_room();
            select_room_win.select_room(room_info);
            // return to lobby
            if (select_room_win.selected_room == room_info.size() + 1)
            {
                status = GameStatus::INITIAL;
                break;
            }
//#ifndef DEBUG
            // create a new room
            else if (select_room_win.selected_room == room_info.size())
            {
                create_win.draw();
                create_win.Render_create_room();
                cr_input_win.draw();
                cr_input_win.get_user_input();
                //send_server_name(sockfd, cr_input_win.name); // send name to server, if room exist, join
            }
            // join a room
            else
            {
                //send_server_room(sockfd, room_info[room_win.selected_room].first);
            }
//#endif
            status = GameStatus::INSIDE_ROOM;
            break;
        case GameStatus::INSIDE_ROOM:
            // inside room
            // room_win.draw();
            // room_win.Render_room();
            // room_win.inside_room();
            status = GameStatus::GAMING;
            break;
        case GameStatus::GAMING:
            noecho(); // disable displaying input
            if (game_loop(&splix_win, &stat_win))
            { // die
                status = GameStatus::GAME_OVER;
            }
            else
            {
                status = GameStatus::INITIAL;
            };
            werase(splix_win.win);
            wrefresh(splix_win.win);
            werase(stat_win.win);
            wrefresh(stat_win.win);
            echo(); // enable displaying input
            break;
        case GameStatus::GAME_OVER:
            // show game over
            curs_set(0);
            gameover_win.render_gameover();
            sleep(3);
            werase(gameover_win.win);
            wrefresh(gameover_win.win);
            status = GameStatus::INITIAL;
            break;
        }
        // after game over
    }
    return 0;
}
