#include "splix_header.h"

int map[MAP_HEIGHT][MAP_WIDTH];
enum class GameStatus
{
    INITIAL,
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
            switch (ch)
            {
            case 'q':
                wattroff(game_win->win, COLOR_PAIR(1) | A_BOLD);
                game_win->exit_game(1);
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
        // WINDOW *win1 = newwin(HEIGHT_INIT_WIN, WIDTH_INIT_WIN, (LINES - HEIGHT_INIT_WIN) / 2, (COLS - WIDTH_INIT_WIN) / 2); // size and location
        // WINDOW *win2 = newwin(HEIGHT_GAME_WIN, WIDTH_GAME_WIN, (LINES - HEIGHT_GAME_WIN) / 2, (COLS - WIDTH_GAME_WIN) / 2);
        // WINDOW *win3 = newwin(HEIGHT_GAME_WIN / 10, WIDTH_GAME_WIN / 4, (LINES - HEIGHT_GAME_WIN) / 2, (COLS + WIDTH_GAME_WIN) / 2);
        GameStatus status = GameStatus::INITIAL;
        Initial_Window init_win(HEIGHT_INIT_WIN, WIDTH_INIT_WIN, (LINES - HEIGHT_INIT_WIN) / 2, (COLS - WIDTH_INIT_WIN) / 2);
        Input_Window input_win(HEIGHT_INIT_WIN / 12, WIDTH_GAME_WIN / 2, (HEIGHT_INIT_WIN - HEIGHT_GAME_WIN / 12) / 2.5, (COLS - WIDTH_GAME_WIN / 2) / 2);
        Status_Window stat_win(HEIGHT_GAME_WIN / 10, WIDTH_GAME_WIN / 4, (LINES - HEIGHT_GAME_WIN) / 2, (COLS + WIDTH_GAME_WIN) / 2);
        Splix_Window splix_win(HEIGHT_GAME_WIN, WIDTH_GAME_WIN, (LINES - HEIGHT_GAME_WIN) / 2, (COLS - WIDTH_GAME_WIN) / 2);
        switch (status)
        {
        case GameStatus::INITIAL:
            init_win.draw();
            init_win.Rendertitle();
            //clear();
            //wrefresh(stdscr);
            input_win.draw();
            input_win.get_user_input();
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
            break;
        }
        // after game over
    }
    endwin();
    return 0;
}
