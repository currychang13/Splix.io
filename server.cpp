#include "splix_header.h"
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <cstring>
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <map>
#include <pthread.h>
#include <sstream>

// Constants
#define MAXCLI 10
#define MAXLINE 4096
#define SERV_PORT 12345
#define LISTENQ 10
#define SA struct sockaddr

// Forward declarations
class Server;

// Forward declaration of GameManager
class GameManager;
// Define client states
enum class ClientState
{
    WAITING_USERNAME,
    ROOM_SELECTION,
    WAITING_START,
    PLAYING_GAME
};

struct GameLoopArgs
{
    int roomId;
    GameManager *gameManager;
};

struct PlayerArgs
{
    Server *server;
    int clientFd;
};

// Structure to pass arguments to the player thread
struct PlayerThreadArgs
{
    int clientFd;
    GameManager *gameManager;
    int roomId;
    int playerId;
};

// Client Information Structure
struct ClientInfo
{
    int fd;
    std::string username;
    int id;
    ClientState state;
};

// Room Structure
struct Room
{
    int roomId;
    std::string roomName;
    std::vector<int> clients; // List of client FDs
};

// RoomManager Class
class RoomManager
{
public:
    RoomManager() : nextRoomId(1) {}

    void initializeRooms()
    {
    }

    void sendRoomInfo(int clientFd, Server &server);

    bool joinRoom(int roomId, int clientFd, Server &server);

    std::map<int, Room> &getAllRooms()
    {
        return rooms;
    }

private:
    std::map<int, Room> rooms;
    int nextRoomId;
};

struct Player
{
    int fd;
    int playerId; // Newly added field for sequential player ID
    int x;
    int y;
    std::pair<int, int> direction; // (dy, dx)
    bool isAlive;
};

struct GameState
{
    int roomId;
    int map[600][600];
    std::map<int, Player> players; // Keyed by client fd
    int nextPlayerId;              // Newly added field for tracking next player ID

    GameState() : nextPlayerId(1) {}
};

class GameManager
{
public:
    GameManager(RoomManager &rm, Server &srv) : roomManager(rm), server(srv) {}

    void startGame(int roomId);
    void handleGameMessage(int roomId, const std::string &message);
    void gameLoop(int roomId);
    bool isGameActive(int roomId);
    void addPlayerToGame(int roomId, int clientFd);
    bool isEnclosure(int y, int x);
    std::vector<std::pair<int, int>> findInsidePoints();
    void fillTerritory(const std::vector<std::pair<int, int>> &inside_points);
    void initializeGameState(int roomId);
    void handlePlayerMessage(int roomId, int clientFd, const std::string &message);
    void handlePlayerDisconnection(int roomId, int clientFd);

private:
    RoomManager &roomManager;
    Server &server;
    std::map<int, GameState> gameStates; // roomId -> GameState
    void updateAllPlayers(int roomId);
    void checkPlayerDeaths(int roomId);
    void broadcastGameState(int roomId);
    void endGame(int roomId);
};

// Server Class
class Server
{
public:
    Server(int port);
    void run();
    void broadcastMessage(const std::string &message, int exclude_fd);
    int getClientRoomId(int clientFd);

    // Accessors for GameManager and RoomManager
    RoomManager &getRoomManager() { return roomManager; }
    GameManager &getGameManager() { return gameManager; }
    pthread_mutex_t &getGameMutex()
    {
        gameMutex = PTHREAD_MUTEX_INITIALIZER;
        return gameMutex;
    }

    const std::map<int, ClientInfo> &getClients() const
    {
        return clients;
    }

private:
    int listenfd;
    int maxfd;
    fd_set allset;
    RoomManager roomManager;
    GameManager gameManager;
    pthread_mutex_t gameMutex;
    int nextUserId;
    std::map<int, ClientInfo> clients; // Map to store client info keyed by file descriptor

    void setupServer(int port);
    void handleNewConnection();
    void processMessage(int clientFd);
    void handleUsername(int clientFd, const std::string &username);
    void handleRoomSelection(int clientFd, const std::string &roomIdStr);
    void handleStartCommand(int clientFd, const std::string &message);
};

// Implementation of RoomManager Methods
void RoomManager::sendRoomInfo(int clientFd, Server &server)
{
    std::stringstream ssr;

    // 1. Send the number of available rooms
    ssr << rooms.size();
    std::string roomSize = ssr.str();

    if (write(clientFd, roomSize.c_str(), roomSize.length()) < 0)
    {
        perror("write error in sendRoomInfo");
    }
    // Only send room information if there are rooms available
    if (!rooms.empty())
    {
        std::stringstream ss;
        for (const auto &[roomId, room] : rooms)
        {
            ss << roomId << " " << room.clients.size() << "\n";
        }

        std::string info = ss.str();

        if (write(clientFd, info.c_str(), info.length()) < 0)
        {
            perror("write error in sendRoomInfo");
        }
    }
    std::cout << "Room info sent to Client FD " << clientFd << "\n";
}

bool RoomManager::joinRoom(int roomId, int clientFd, Server &server)
{
    auto it = rooms.find(roomId);
    if (it != rooms.end())
    {
        // Room exists. Add client to the room.
        it->second.clients.push_back(clientFd);
        std::cout << "Client FD " << clientFd << " joined existing room " << roomId << std::endl;

        // Prepare the list of other users' usernames
        std::stringstream ss;
        const auto &clientsMap = server.getClients(); // Using the getter
        for (const auto &fd : it->second.clients)
        {
            if (fd != clientFd)
            {
                auto clientIt = clientsMap.find(fd);
                if (clientIt != clientsMap.end())
                {
                    ss << clientIt->second.username << "\n";
                    std::string userList = ss.str();

                    // Send the usernames to the joining client
                    ssize_t bytesSent = write(clientFd, userList.c_str(), userList.length());
                    if (bytesSent < 0)
                    {
                        perror("write error in joinRoom");
                        return false;
                    }
                }
            }
        }

        return true;
    }
    else
    {
        // Room does not exist. Create a new room with the specified roomId.
        Room newRoom;
        newRoom.roomId = roomId;
        newRoom.roomName = "Room_" + std::to_string(roomId); // You can customize the naming convention
        newRoom.clients.push_back(clientFd);
        rooms[roomId] = newRoom;
        std::cout << "Client FD " << clientFd << " created and joined new room " << roomId << std::endl;
        return true;
    }
}

static void *gameLoopWrapper(void *arg)
{
    GameLoopArgs *args = static_cast<GameLoopArgs *>(arg);
    int roomId = args->roomId;
    GameManager *gm = args->gameManager;
    delete args; // Prevent memory leak

    gm->gameLoop(roomId);

    return nullptr;
}

// Implementation of GameManager Methods
void GameManager::startGame(int roomId)
{
    // Create a new thread for the game loop
    pthread_t thread;
    GameLoopArgs *args = new GameLoopArgs();
    args->roomId = roomId;
    args->gameManager = this;

    if (pthread_create(&thread, nullptr, gameLoopWrapper, args) != 0)
    {
        std::cerr << "Failed to create game thread for room " << roomId << std::endl;
        delete args;
        return;
    }

    pthread_detach(thread);
}

void GameManager::initializeGameState(int roomId)
{
    std::cout << "Initializing game state for room " << roomId << std::endl;

    // Create a new GameState
    GameState gameState;
    gameState.roomId = roomId;

    // Initialize the 600x600 map to 0
    for (int i = 0; i < MAP_WIDTH; ++i)
    {
        for (int j = 0; j < MAP_HEIGHT; ++j)
        {
            gameState.map[i][j] = 0;
        }
    }
    // Store the game state
    gameStates[roomId] = gameState;
}

void *playerThreadFunction(void *args)
{
    PlayerThreadArgs *Playerargs = (PlayerThreadArgs *) args;
    char buffer[1024];
    while (true)
    {
        ssize_t bytesRead = read(Playerargs->clientFd, buffer, sizeof(buffer) - 1);
        if (bytesRead <= 0)
        {
            if (bytesRead < 0)
            {
                perror("read error in playerThreadFunction");
            }
            else
            {
                std::cout << "Client FD " << Playerargs->clientFd << " disconnected.\n";
            }

            // Handle player disconnection
            Playerargs->gameManager->handlePlayerDisconnection(Playerargs->roomId, Playerargs->clientFd);
            close(Playerargs->clientFd);
            return nullptr;
        }

        buffer[bytesRead] = '\0';
        std::string message(buffer);

        // Handle the received message
        Playerargs->gameManager->handlePlayerMessage(Playerargs->roomId, Playerargs->clientFd, message);
    }
}

void GameManager::addPlayerToGame(int roomId, int clientFd)
{
    pthread_mutex_lock(&server.getGameMutex());

    auto &gameState = gameStates[roomId];
    Player player;
    player.fd = clientFd;
    player.direction = {0, 1}; // Default direction: moving right
    player.isAlive = true;

    // Assign a sequential playerId based on join order
    int playerId = gameState.nextPlayerId++;
    player.playerId = playerId;

    // Generate a random starting position with a minimum distance from walls
    srand(time(NULL) + clientFd); // Seed with current time and clientFd for uniqueness
    const int MIN_DISTANCE_FROM_WALL = 30;

    int start_x = MIN_DISTANCE_FROM_WALL + rand() % (MAP_WIDTH - 2 * MIN_DISTANCE_FROM_WALL);
    int start_y = MIN_DISTANCE_FROM_WALL + rand() % (MAP_HEIGHT - 2 * MIN_DISTANCE_FROM_WALL);

    // Ensure the starting position is on an empty cell
    while (gameState.map[start_y][start_x] != 0)
    {
        start_x = MIN_DISTANCE_FROM_WALL + rand() % (MAP_WIDTH - 2 * MIN_DISTANCE_FROM_WALL);
        start_y = MIN_DISTANCE_FROM_WALL + rand() % (MAP_HEIGHT - 2 * MIN_DISTANCE_FROM_WALL);
    }

    player.x = start_x;
    player.y = start_y;

    // Assign the player's ID to the map
    gameState.map[player.y][player.x] = playerId;

    // Mark the surrounding 8 cells as the player's territory (-playerId)
    for (int dy = -1; dy <= 1; ++dy)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            int new_x = player.x + dx;
            int new_y = player.y + dy;

            // Check map boundaries
            if (new_x >= 0 && new_x < MAP_WIDTH && new_y >= 0 && new_y < MAP_HEIGHT)
            {
                // Only set if the cell is empty
                if (gameState.map[new_y][new_x] == 0)
                {
                    gameState.map[new_y][new_x] = -playerId;
                }
            }
        }
    }

    // Add player to game state
    gameState.players[clientFd] = player;

    // Prepare arguments for player thread
    PlayerThreadArgs *ptArgs = new PlayerThreadArgs();
    ptArgs->clientFd = clientFd;
    ptArgs->gameManager = this;
    ptArgs->roomId = roomId;
    ptArgs->playerId = playerId;

    // Create a new thread for the player
    pthread_t playerThread;
    if (pthread_create(&playerThread, nullptr, playerThreadFunction, ptArgs) != 0)
    {
        std::cerr << "Failed to create thread for Player ID " << playerId << " (Client FD " << clientFd << ")\n";
        delete ptArgs;
        // Handle thread creation failure (e.g., remove player from game)
        pthread_mutex_unlock(&server.getGameMutex());
        return;
    }

    pthread_detach(playerThread); // Detach the thread to allow independent execution

    std::cout << "Player ID " << playerId << " (Client FD " << clientFd << ") added to Room " << roomId << "\n";

    pthread_mutex_unlock(&server.getGameMutex());
}

void GameManager::handlePlayerMessage(int roomId, int clientFd, const std::string &message)
{
    pthread_mutex_lock(&server.getGameMutex());

    // Parse and handle the message
    // Example: Move command "MOVE UP", "MOVE DOWN", etc.
    if (message.find("MOVE") == 0)
    {
        std::string direction = message.substr(5); // Extract direction

        auto &gameState = gameStates[roomId];
        auto it = gameState.players.find(clientFd);
        if (it != gameState.players.end() && it->second.isAlive)
        {
            if (direction == "UP")
            {
                it->second.direction = {-1, 0};
            }
            else if (direction == "DOWN")
            {
                it->second.direction = {1, 0};
            }
            else if (direction == "LEFT")
            {
                it->second.direction = {0, -1};
            }
            else if (direction == "RIGHT")
            {
                it->second.direction = {0, 1};
            }
            else
            {
                std::string errorMsg = "Invalid direction. Use UP, DOWN, LEFT, or RIGHT.\n";
                write(clientFd, errorMsg.c_str(), errorMsg.length());
            }
        }
    }
    // Handle other message types as needed

    pthread_mutex_unlock(&server.getGameMutex());
}

void GameManager::handlePlayerDisconnection(int roomId, int clientFd)
{
    pthread_mutex_lock(&server.getGameMutex());

    auto &gameState = gameStates[roomId];
    auto it = gameState.players.find(clientFd);
    if (it != gameState.players.end())
    {
        // Clear the player's path and territory from the map
        gameState.map[it->second.y][it->second.x] = 0;

        // Optionally, remove surrounding territory or handle game-specific logic
        // ...

        std::cout << "Player ID " << it->second.playerId << " (Client FD " << clientFd << ") disconnected from Room " << roomId << "\n";

        // Remove player from the game state
        gameState.players.erase(it);

        // Optionally, notify other players in the room
        std::string notifyMsg = "Player " + std::to_string(it->second.playerId) + " has disconnected.\n";
    }

    pthread_mutex_unlock(&server.getGameMutex());
}


bool GameManager::isEnclosure(int y, int x)
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
            if (gameStates[roomId].map[ny][nx] == id || map[ny][nx] == -id)
            {
                visited[ny][nx] = true;
                q.push({ny, nx});
            }
        }
    }

    // If the component touches the border, it's not an enclosure
    return !touches_border;
}
std::vector<std::pair<int, int>> GameManager::findInsidePoints()
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
void GameManager::fillTerritory(const std::vector<std::pair<int, int>> &inside_points)
{
    for (const auto &[y, x] : inside_points)
    {
        map[y][x] = -id; // Mark as filled territory
    }
}

void GameManager::handleGameMessage(int roomId, const std::string &message)
{
    // Example: Update game state based on the action
}

void GameManager::gameLoop(int roomId)
{
    pthread_mutex_lock(&server.getGameMutex());

    // Update game state
    updateAllPlayers(roomId);

    // Check for player deaths
    checkPlayerDeaths(roomId);

    // Broadcast game state to all clients
    broadcastGameState(roomId);

    pthread_mutex_unlock(&server.getGameMutex());

    sleep(1); // Simulate game ticks

    endGame(roomId);
}

void GameManager::updateAllPlayers(int roomId)
{
    auto &gameState = gameStates[roomId];
    for (auto &[fd, player] : gameState.players)
    {
        if (player.isAlive)
        {
            // Calculate new position
            int newX = player.x + player.direction.second;
            int newY = player.y + player.direction.first;

            // Boundary checks
            if (newX < 0 || newX >= MAP_WIDTH || newY < 0 || newY >= MAP_HEIGHT)
            {
                player.isAlive = false;
                gameState.map[player.y][player.x] = 0;
                continue;
            }

            const auto &clientsMap = server.getClients(); // Using the getter
            auto clientIt = clientsMap.find(fd);

            // Update map
            gameState.map[player.y][player.x] = 0; // Clear old position
            player.x = newX;
            player.y = newY;
            gameState.map[player.y][player.x] = clientIt->second.id; // Set new position
        }
    }
}

void GameManager::checkPlayerDeaths(int roomId)
{
    auto &gameState = gameStates[roomId];
    std::map<std::pair<int, int>, std::vector<int>> positionMap;
    for (auto &[fd, player] : gameState.players)
    {
        if (player.isAlive)
        {
            positionMap[{player.x, player.y}].push_back(fd);
        }
    }

    for (auto &[position, fds] : positionMap)
    {
        if (fds.size() > 1)
        {
            // All players at this position die
            for (auto &fd : fds)
            {
                gameState.players[fd].isAlive = false;
                gameState.map[position.second][position.first] = 0;
                std::string deathMsg = "You have died at position (" +
                                       std::to_string(position.first) + ", " +
                                       std::to_string(position.second) + ").\n";
                write(fd, deathMsg.c_str(), deathMsg.length());
            }
        }
    }
}

void GameManager::broadcastGameState(int roomId)
{
    auto &gameState = gameStates[roomId];
    std::string stateMsg = "GAME_STATE\n";
    for (auto &[fd, player] : gameState.players)
    {
        stateMsg += "Player " + std::to_string(player.fd) + ": (" +
                    std::to_string(player.x) + ", " +
                    std::to_string(player.y) + ") Alive: " +
                    (player.isAlive ? "Yes" : "No") + "\n";
    }
    Room &room = roomManager.getAllRooms().at(roomId);
    for (auto &fd : room.clients)
    {
        write(fd, stateMsg.c_str(), stateMsg.length());
    }
}

bool GameManager::isGameActive(int roomId)
{
    return gameStates.find(roomId) != gameStates.end();
}

void GameManager::endGame(int roomId)
{
    if (!isGameActive(roomId))
    {
        std::cout << "No active game in room " << roomId << std::endl;
        return;
    }

    gameStates.erase(roomId);
    std::cout << "Game ended in room " << roomId << std::endl;

    Room &room = roomManager.getAllRooms().at(roomId);
    std::string endMsg = "GAME_END\n";
    for (auto &fd : room.clients)
    {
        write(fd, endMsg.c_str(), endMsg.length());
    }
}

// Implementation of Server Methods
Server::Server(int port)
    : listenfd(-1), maxfd(-1), roomManager(), gameManager(roomManager, *this),
      nextUserId(1)
{
    FD_ZERO(&allset);
    setupServer(port);
    roomManager.initializeRooms();
}

void Server::run()
{
    fd_set rset;
    while (true)
    {
        rset = allset;
        int nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (nready < 0)
        {
            perror("select error");
            break;
        }

        if (FD_ISSET(listenfd, &rset))
        {
            handleNewConnection();
            if (--nready <= 0)
                continue;
        }

        for (const auto &[fd, ci] : clients)
        {
            if (FD_ISSET(fd, &rset))
            {
                processMessage(fd);
                if (--nready <= 0)
                    break;
            }
        }
    }
}

void Server::setupServer(int port)
{
    struct sockaddr_in servaddr;

    // Create a TCP socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0)
    {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    // Set socket options to reuse the address
    int opt = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt error");
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    // Zero out the server address structure
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;                // IPv4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // Listen on any interface
    servaddr.sin_port = htons(port);              // Server port

    // Bind the socket to the address and port
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("bind error");
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(listenfd, LISTENQ) < 0)
    {
        perror("listen error");
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    // Initialize the file descriptor set
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    maxfd = listenfd;

    std::cout << "Server setup complete. Listening on port " << port << std::endl;
}

void Server::handleNewConnection()
{
    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);
    int connfd = accept(listenfd, (SA *)&cliaddr, &clilen);
    if (connfd < 0)
    {
        perror("accept error");
        return;
    }

    // Initialize ClientInfo with state WAITING_USERNAME
    ClientInfo ci;
    ci.fd = connfd;
    ci.username = "";
    ci.id = nextUserId++;
    ci.state = ClientState::WAITING_USERNAME;

    // Store the client information
    clients[connfd] = ci;

    // Add the new socket to the set
    FD_SET(connfd, &allset);
    if (connfd > maxfd)
        maxfd = connfd;

    std::cout << "New connection established. FD: " << connfd << std::endl;
}

// Broadcast a message to all clients except exclude_fd
void Server::broadcastMessage(const std::string &message, int exclude_fd)
{
    for (const auto &[fd, ci] : clients)
    {
        if (fd != exclude_fd)
        {
            write(fd, message.c_str(), message.length());
        }
    }
}

// Process incoming messages based on client state
void Server::processMessage(int clientFd)
{
    char recvline[MAXLINE];
    ssize_t n = read(clientFd, recvline, MAXLINE - 1);
    if (n <= 0)
    {
        // Handle client disconnect
        if (clients.find(clientFd) != clients.end())
        {
            std::cout << "Client " << clients[clientFd].username << " disconnected." << std::endl;
            close(clientFd);
            FD_CLR(clientFd, &allset);
            clients.erase(clientFd);

            // Optionally, notify others in the room
            // ... (Implement if needed)
        }
        return;
    }
    recvline[n] = '\0';
    std::string message(recvline);

    // Trim any trailing newline or carriage return characters
    message.erase(std::remove(message.begin(), message.end(), '\n'), message.end());
    message.erase(std::remove(message.begin(), message.end(), '\r'), message.end());

    // Retrieve the client's current state
    ClientState state = clients[clientFd].state;

    switch (state)
    {
    case ClientState::WAITING_USERNAME:
        handleUsername(clientFd, message);
        break;

    case ClientState::ROOM_SELECTION:
        handleRoomSelection(clientFd, message);
        break;

    case ClientState::WAITING_START:
        handleStartCommand(clientFd, message);
        break;

    case ClientState::PLAYING_GAME:
        // Delegate game-related messages to GameManager
        gameManager.handleGameMessage(getClientRoomId(clientFd), message);
        break;

    default:
        std::cerr << "Unknown state for client FD " << clientFd << std::endl;
        break;
    }
}

// Handle username input
void Server::handleUsername(int clientFd, const std::string &username)
{

    // Update client's username and state
    clients[clientFd].username = username;
    clients[clientFd].state = ClientState::ROOM_SELECTION;

    std::cout << "Client FD " << clientFd << " set username to " << username << std::endl;

    // Send room information to the client
    roomManager.sendRoomInfo(clientFd, *this);
}

// Handle room selection
void Server::handleRoomSelection(int clientFd, const std::string &roomIdStr)
{
    // if (!std::all_of(roomIdStr.begin(), roomIdStr.end(), ::isdigit))
    // {
    //     std::string error = "Invalid Room ID. Please enter a numeric Room ID:\n";
    //     write(clientFd, error.c_str(), error.length());
    //     return;
    // }

    int selectedRoomId = std::stoi(roomIdStr);
    bool joined = roomManager.joinRoom(selectedRoomId, clientFd, *this);
    if (joined)
    {
        // Update client's state
        clients[clientFd].state = ClientState::WAITING_START;
    }
}

// Handle start command
void Server::handleStartCommand(int clientFd, const std::string &message)
{
    if (message != "start")
    {
        // Update client's state back to ROOM_SELECTION
        clients[clientFd].state = ClientState::ROOM_SELECTION;

        // Send room information to the client
        roomManager.sendRoomInfo(clientFd, *this);
        return;
    }

    int roomId = getClientRoomId(clientFd);
    if (roomId == -1)
    {
        std::string error = "You are not in any room.\n";
        ssize_t bytesWritten = write(clientFd, error.c_str(), error.length());
        if (bytesWritten < 0)
        {
            perror("write error in handleStartCommand");
        }
        return;
    }

    clients[clientFd].state = ClientState::PLAYING_GAME;
    gameManager.initializeGameState(roomId);
    gameManager.addPlayerToGame(roomId, clientFd);

    // Start the game in GameManager
    gameManager.startGame(roomId);

    std::cout << "Client FD " << clientFd << " started game in Room " << roomId << "\n";
}

// Get the room ID of a client
int Server::getClientRoomId(int clientFd)
{
    for (const auto &[roomId, room] : roomManager.getAllRooms())
    {
        if (std::find(room.clients.begin(), room.clients.end(), clientFd) != room.clients.end())
        {
            return roomId;
        }
    }
    return -1; // Not found
}

// Main Function
int main()
{
    Server server(SERV_PORT);
    server.run();
    return 0;
}