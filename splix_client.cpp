#include "splix_header.h"
// #define DEBUG
// colors
#define COLOR_CORAL 15
#define COLOR_PURPLE 16
#define COLOR_TEAL 17
#define COLOR_DEEPGRAY 18
#define COLOR_GRAY 19

int map[MAP_HEIGHT][MAP_WIDTH];
Player player;
TcpContent tcp;
UdpContent udp;
std::vector<std::pair<int, int>> room_info;
std::vector<std::string> member_info;
std::queue<std::string> message_queue;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for queue

std::pair<int, int> unbox(std::string str, Mode &mode, int &value) // value mode head_x head_y x y x y x y
{
    std::stringstream ss(str);
    std::string token, tokenY, tokenX;
    ss >> token;
    value = std::stoi(token);
    // std::cerr << value;
    // napms(2000);

    ss >> token;
    mode = token == "FAST" ? Mode::FAST : Mode::NORMAL;

    ss >> token;
    int head_x, head_y;
    head_y = std::stoi(token);

    ss >> token;
    head_x = std::stoi(token);

    map[head_y][head_x] = value;

    while (ss >> tokenY >> tokenX)
    {
        int y = std::stoi(tokenY);
        int x = std::stoi(tokenX);
        map[y][x] = value;
    }
    return {head_y, head_x};
}

void *listen_to_server(void *arg)
{
    pthread_detach(pthread_self());
    int sockfd = *(int *)arg;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);

    while (1)
    {
        ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0)
        {
            buffer[bytes_received] = '\0';
            pthread_mutex_lock(&queue_mutex);
            message_queue.push(std::string(buffer));
            pthread_mutex_unlock(&queue_mutex);
        }
        else if (bytes_received == 0)
        {
            printf("Connection closed by server.\n");
            break;
        }
        else
        {
            perror("recvfrom error");
            break;
        }
    }
    pthread_exit(NULL);
}

bool game_loop(Splix_Window *game_win, Status_Window *stat_win)
{
    nodelay(game_win->win, TRUE);
    keypad(game_win->win, TRUE);
    setlocale(LC_ALL, "");
    message_queue = std::queue<std::string>();

    for (int i = 0; i < MAP_HEIGHT; i++)
    {
        for (int j = 0; j < MAP_WIDTH; j++)
        {
            map[i][j] = 0;
        }
    }

#ifndef DEBUG
    udp.udp_connect();
    std::pair<int, int> position = udp.get_position_from_server();
    player.init(position, {0, 1}, udp.get_id_from_server(), Mode::NORMAL, acc_time, 0, 0);
    game_win->id = player.id;

    pthread_t server_thread;
    if (pthread_create(&server_thread, NULL, listen_to_server, &udp.sockfd) != 0)
    {
        perror("Failed to create server listener thread");
        return false;
    }
#endif

#ifdef DEBUG
    player.init({rand() % (MAP_HEIGHT - 20), rand() % (MAP_WIDTH - 20)}, {0, 1}, 9, Mode::NORMAL, acc_time, 0, 0);
    game_win->id = player.id;
#endif

    game_win->draw();
    game_win->create_initial_territory(player.coordinate_y, player.coordinate_x);
    game_win->render_game(player.coordinate_y, player.coordinate_x, player.mode, player, player.id);
    stat_win->draw();
    stat_win->update_status(player.coordinate_y, player.coordinate_x, "NORMAL", player.id);
    stat_win->update_timer(player.acceleration_timer, player.cooldown_timer);
    wrefresh(stat_win->win);
    // Start the ticker
    GameTicker ticker_slow(5);
    GameTicker ticker_normal(15);
    GameTicker ticker_fast(45);

    while (true)
    {
#ifndef DEBUG

        if (!message_queue.empty())
        {
            pthread_mutex_lock(&queue_mutex);
            std::string cur_str = message_queue.front();
            message_queue.pop();
            pthread_mutex_unlock(&queue_mutex);
            int id;
            std::pair<int, int> head;
            Mode mode;
            head = unbox(cur_str, mode, id);
            game_win->render_game(head.first, head.second, mode, player, id);
        }
#endif
        if ((player.mode == Mode::NORMAL && ticker_normal.is_tick_due()) ||
            (player.mode == Mode::FAST && ticker_fast.is_tick_due()) || ticker_slow.is_tick_due() && player.mode == Mode::SLOW)
        {
            int ch = wgetch(game_win->win);
            std::pair<int, int> new_direction = player.direction;
            flushinp();
            if (ch != ERR)
            {
                switch (ch)
                {
                case 'q':
                    wattroff(game_win->win, COLOR_PAIR(player.id) | A_BOLD);
                    udp.send_leave_game();
                    game_win->exit_game(1);
                    return false;
                case 'w':
                case KEY_UP:
                    new_direction = {-1, 0};
                    break;
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
                    if (player.mode == Mode::FAST)
                    {
                        player.mode = Mode::NORMAL;
                    }
                    else if (player.cooldown_timer == 0)
                    {
                        player.mode = Mode::FAST;
                        if (player.acceleration_timer == 0)
                            player.acceleration_timer = acc_time;
                    }
                    break;
                case 'g':
                    if (player.mode == Mode::SLOW && player.cooldown_timer == 0)
                    {
                        player.mode = Mode::NORMAL;
                    }
                    else
                    {
                        player.mode = Mode::SLOW;
                    }
                    break;
                default:
                    break;
                }
                if (new_direction.first != -player.direction.first &&
                    new_direction.second != -player.direction.second)
                {
                    player.direction = new_direction;
                }
            }

            // Handle timers
            if (player.mode == Mode::FAST && player.acceleration_timer > 0)
            {
                player.acceleration_timer--;
                if (player.acceleration_timer == 0)
                {
                    player.mode = Mode::SLOW;
                    player.cooldown_timer = cool_time;
                }
            }
            if ((player.mode == Mode::NORMAL || player.mode == Mode::SLOW) && player.acceleration_timer > 0 && player.acceleration_timer < acc_time)
            {
                player.acceleration_timer++;
            }
            else if (player.cooldown_timer > 0)
            {
                player.cooldown_timer--;
                if (player.cooldown_timer == 0)
                {
                    player.acceleration_timer = acc_time;
                    player.mode = Mode::NORMAL;
                }
            }

            // Update position
            player.coordinate_y += player.direction.first;
            player.coordinate_x += player.direction.second;

            // send to server
            udp.send_server_position(player.coordinate_y, player.coordinate_x, player.id, player.mode);

            // Check if the player dies from going out of bounds
            if (player.coordinate_y < 1 || player.coordinate_y >= MAP_HEIGHT - 1 || player.coordinate_x < 1 || player.coordinate_x >= MAP_WIDTH - 1 || map[player.coordinate_y][player.coordinate_x] == player.id)
            {
                udp.send_leave_game();
                game_win->exit_game(0); // die
                return true;
            }
            // Handle territory and update map
            if (map[player.coordinate_y][player.coordinate_x] == -player.id)
            {
                if (game_win->is_enclosure(player.coordinate_y, player.coordinate_x))
                {
                    auto inside_points = game_win->find_inside_points();
                    std::string message = "";
                    game_win->fill_territory(inside_points);
                }
            }
            else
                map[player.coordinate_y][player.coordinate_x] = player.id;

            game_win->render_game(player.coordinate_y, player.coordinate_x, player.mode, player, player.id);
            stat_win->update_status(player.coordinate_y, player.coordinate_x,
                                    (player.mode == Mode::FAST) ? "BURST" : (player.mode == Mode::NORMAL) ? "NORMAL"
                                                                                                          : "SLOW",
                                    player.id);
            stat_win->update_timer(player.acceleration_timer, player.cooldown_timer);
            wrefresh(stat_win->win);
        }
    }
#ifndef DEBUG
    pthread_cancel(server_thread);
    close(udp.sockfd);
#endif
    return false;
}

int main()
{
    setlocale(LC_ALL, "");
    initscr();
    start_color();
    use_default_colors();
    cbreak();
    keypad(stdscr, TRUE);
    init_color(COLOR_GRAY, 500, 500, 500);
    init_color(COLOR_PURPLE, 800, 400, 900);   // Light Purple
    init_color(COLOR_TEAL, 200, 700, 700);     // Light Teal
    init_color(COLOR_CORAL, 1000, 500, 400);   // Coral
    init_color(COLOR_DEEPGRAY, 300, 300, 300); // Dark Gray
    init_color(COLOR_WHITE, 1000, 1000, 1000); // White

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

    // windows
    Initial_Window init_win(HEIGHT_INIT_WIN, WIDTH_INIT_WIN, (LINES - HEIGHT_INIT_WIN) / 2, (COLS - WIDTH_INIT_WIN) / 2);
    Rule_Window rule_win(13, 37, (LINES - 7) / 2, (COLS - 37) / 2);
    Input_Window input_win(HEIGHT_INIT_WIN / 13, WIDTH_GAME_WIN / 3, (HEIGHT_INIT_WIN - HEIGHT_GAME_WIN / 13) / 2.5, (COLS - WIDTH_GAME_WIN / 3) / 2);

    Select_Room_Window select_room_win(HEIGHT_INIT_WIN, WIDTH_INIT_WIN, (LINES - HEIGHT_INIT_WIN) / 2, (COLS - WIDTH_INIT_WIN) / 2);

    Create_Room_Window create_win(HEIGHT_INIT_WIN, WIDTH_INIT_WIN, (LINES - HEIGHT_INIT_WIN) / 2, (COLS - WIDTH_INIT_WIN) / 2);
    CR_Input_Window cr_input_win(HEIGHT_INIT_WIN / 13, WIDTH_INIT_WIN / 3, (HEIGHT_INIT_WIN - HEIGHT_INIT_WIN / 12) / 2.5, (COLS - WIDTH_INIT_WIN / 3) / 2);

    Room_Window room_win(HEIGHT_INIT_WIN, WIDTH_INIT_WIN, (LINES - HEIGHT_INIT_WIN) / 2, (COLS - WIDTH_INIT_WIN) / 2);

    Status_Window stat_win(HEIGHT_GAME_WIN / 5, WIDTH_GAME_WIN / 4, (LINES - HEIGHT_GAME_WIN) / 2, (COLS + WIDTH_GAME_WIN) / 2);
    Splix_Window splix_win(HEIGHT_GAME_WIN, WIDTH_GAME_WIN, (LINES - HEIGHT_GAME_WIN) / 2, (COLS - WIDTH_GAME_WIN) / 2);

    Gameover_Window gameover_win(HEIGHT_GAME_WIN, WIDTH_GAME_WIN, (LINES - HEIGHT_GAME_WIN) / 2, (COLS - WIDTH_GAME_WIN) / 2);

#ifdef DEBUG
    room_info.push_back({1, 2});
    room_info.push_back({2, 3});
    room_info.push_back({3, 4});
    room_info.push_back({4, 5});
    room_info.push_back({5, 6});
    room_info.push_back({6, 7});
    room_info.push_back({7, 8});
    room_info.push_back({8, 9});
    room_info.push_back({9, 10});
    room_info.push_back({10, 11});
    room_info.push_back({11, 12});
    room_info.push_back({12, 13});
    room_info.push_back({13, 14});
    room_info.push_back({14, 15});
    room_info.push_back({15, 16});
    member_info.push_back("Mace6728");
    member_info.push_back("Droplet5269");
    member_info.push_back("humphrey");
    member_info.push_back("Cubeman");
    member_info.push_back("Mace6728");
    member_info.push_back("Droplet5269");
    member_info.push_back("humphrey");
    member_info.push_back("Cubeman");
    member_info.push_back("Mace6728");
    member_info.push_back("Droplet5269");
#endif
    while (true)
    {
        switch (status)
        {
        case GameStatus::INITIAL:
#ifndef DEBUG
            member_info.clear();
            room_info.clear();
#endif
            init_win.draw();
            init_win.Rendertitle();
            init_win.Show_Instruction();
            wrefresh(init_win.win);
            rule_win.draw();
            rule_win.Show_Rules();
            input_win.draw();
            input_win.get_user_input(); // can't not get empty name
            strcpy(player.name, input_win.name);
#ifndef DEBUG
            tcp.tcp_connect();
            tcp.send_server_name(player.name);
#endif
            status = GameStatus::ROOM_SELECTION;
            break;

        case GameStatus::ROOM_SELECTION:
#ifndef DEBUG
            room_info.clear();
            tcp.receive_room_info(room_info);
#endif
            select_room_win.clean();
            select_room_win.draw();
            select_room_win.Render_select_room();
            select_room_win.select_room(room_info);

            if (select_room_win.selected_room == room_info.size() + 1)
            {
                status = GameStatus::INITIAL;
#ifndef DEBUG
                close(tcp.sockfd);
#endif
                break;
            }

            else if (select_room_win.selected_room == room_info.size())
            {
                if (room_info.size() >= ROOM_LIMIT)
                {
                    Window full_win(5, 30, (LINES - 5) / 2, (COLS - 30) / 2);
                    full_win.draw();
                    mvwprintw(full_win.win, 2, (full_win.width - 17) / 2, "Room is full now");
                    wgetch(full_win.win);
                    full_win.clean();
                    status = GameStatus::ROOM_SELECTION;
                    break;
                }
                else
                {
                    create_win.clean();
                    create_win.draw();
                    create_win.Render_create_room();
                    cr_input_win.draw();
                    create_win.Show_Instruction();
                    wrefresh(create_win.win);
                    cr_input_win.get_user_input();
                    create_win.Show_choice();
                    if (create_win.selected_choice == 1)
                    {
                        player.room_id = atoi(cr_input_win.id);
                    }
                    else
                    {
                        status = GameStatus::ROOM_SELECTION;
                        break;
                    }
                }
#ifndef DEBUG
                tcp.send_server_room_id(player.room_id); // send id to server, if room exist, join
#endif
            }

            else
            {
                player.room_id = room_info[select_room_win.selected_room].first;
#ifndef DEBUG
                tcp.send_server_room_id(room_info[select_room_win.selected_room].first);
#endif
            }

            status = GameStatus::INSIDE_ROOM;
            break;

        case GameStatus::INSIDE_ROOM:
            room_win.draw();
            room_win.Render_room();
#ifndef DEBUG
            member_info.clear();
            tcp.receive_member_info(member_info);
#endif
            room_win.inside_room(member_info, player.room_id);
            if (room_win.selected_object == member_info.size())
            {
                status = GameStatus::GAMING;
            }
            else if (room_win.selected_object == member_info.size() + 1)
            {
                status = GameStatus::ROOM_SELECTION;
#ifndef DEBUG
                tcp.send_return_to_room_selection();
#endif
            }
            break;

        case GameStatus::GAMING:
            noecho();
#ifndef DEBUG
            tcp.send_start();
            tcp.receive_port(udp.port);
#endif
            if (game_loop(&splix_win, &stat_win))
            {
                status = GameStatus::GAME_OVER;
            }
            else
            {
                status = GameStatus::INSIDE_ROOM;
            };
            werase(splix_win.win);
            wrefresh(splix_win.win);
            werase(stat_win.win);
            wrefresh(stat_win.win);
            echo();
            break;

        case GameStatus::GAME_OVER:
            // show game over
            curs_set(0);
            gameover_win.render_gameover();
            sleep(3);
            werase(gameover_win.win);
            wrefresh(gameover_win.win);
            status = GameStatus::INSIDE_ROOM;
            break;
        }
        // after game over
    }
    return 0;
}
