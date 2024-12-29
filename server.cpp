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

    int createRoom(const std::string &roomName)
    {
        int roomId = nextRoomId++;
        Room newRoom;
        newRoom.roomId = roomId;
        newRoom.roomName = roomName;
        rooms[roomId] = newRoom;
        std::cout << "Room created: " << roomName << " with ID " << roomId << std::endl;
        return roomId;
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

// GameManager Class
struct Player
{
    int fd;
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
};

class GameManager
{
public:
    GameManager(RoomManager &rm, Server &srv) : roomManager(rm), server(srv) {}

    void startGame(int roomId);
    void handleGameMessage(int roomId, const std::string &message);
    void gameLoop(int roomId);

private:
    RoomManager &roomManager;
    Server &server;
    std::map<int, GameState> gameStates; // roomId -> GameState

    void initializeGameState(int roomId);
    void addPlayerToGame(int roomId, int clientFd);
    void updateAllPlayers(int roomId);
    void checkPlayerDeaths(int roomId);
    void broadcastGameState(int roomId);
    bool isGameActive(int roomId);
    void endGame(int roomId);
};

void GameManager::addPlayerToGame(int roomId, int clientFd)
{
    pthread_mutex_lock(&server.getGameMutex());

    auto &gameState = gameStates[roomId];
    Player player;
    player.fd = clientFd;
    player.x = rand() % MAP_WIDTH;
    player.y = rand() % MAP_HEIGHT;
    player.direction = {0, 1}; // Default direction
    player.isAlive = true;

    // Add player to game state
    gameState.players[clientFd] = player;
    gameState.map[player.y][player.x] = server.getClients().at(clientFd).id;

    // Notify the client of their initial position
    std::string initMsg = "INIT " + std::to_string(player.x) + " " + std::to_string(player.y) + "\n";
    write(clientFd, initMsg.c_str(), initMsg.length());

    pthread_mutex_unlock(&server.getGameMutex());
}

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
    pthread_mutex_t &getGameMutex() { return gameMutex; }

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
    char buf[MAXLINE];
    read(clientFd, buf, MAXLINE);
    std::cout << buf << "\n";
    std::stringstream ss;
    // 2. Send each room's ID and the number of users in the room
    for (const auto &[roomId, room] : rooms)
    {
        ss << roomId << " " << room.clients.size();
    }

    // Convert the stringstream to a string
    std::string info = ss.str();

    // Send the information to the client
    if (write(clientFd, info.c_str(), info.length()) < 0)
    {
        perror("write error in sendRoomInfo");
    }
}

// FILE: server.cpp

bool RoomManager::joinRoom(int roomId, int clientFd, Server &server)
{
    auto it = rooms.find(roomId);
    if (it != rooms.end())
    {
        // Room exists. Add client to the room.
        it->second.clients.push_back(clientFd);
        std::cout << "Client FD " << clientFd << " joined existing room " << roomId << std::endl;

        if (server.getGameManager().isGameActive(roomId))
        {
            server.getGameManager().addPlayerToGame(roomId, clientFd);
        }

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

        // Notify the client that a new room has been created and joined
        std::string ack = "none";
        ssize_t bytesSent = write(clientFd, ack.c_str(), ack.length());
        if (bytesSent < 0)
        {
            perror("write error in joinRoom (new room creation)");
            return false;
        }

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
    if (isGameActive(roomId))
    {
        std::cout << "Game already active in room " << roomId << std::endl;
        return;
    }

    initializeGameState(roomId);

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

    std::cout << "Game started in room " << roomId << std::endl;
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

    // Get all clients in the room
    Room &room = roomManager.getAllRooms().at(roomId);
    srand(time(NULL));

    // Assign each player a random starting position and direction
    for (auto &fd : room.clients)
    {
        Player player;
        player.fd = fd;
        player.x = rand() % MAP_WIDTH;
        player.y = rand() % MAP_HEIGHT;
        // Assign a random direction: up, down, left, right
        int dir = rand() % 4;
        switch (dir)
        {
        case 0:
            player.direction = {-1, 0};
            break; // Up
        case 1:
            player.direction = {1, 0};
            break; // Down
        case 2:
            player.direction = {0, -1};
            break; // Left
        case 3:
            player.direction = {0, 1};
            break; // Right
        }
        player.isAlive = true;

        // Update the map with player's initial position
        const auto &clientsMap = server.getClients(); // Using the getter
        auto clientIt = clientsMap.find(fd);
        gameState.map[player.y][player.x] = clientIt->second.id;

        // Add player to game state
        gameState.players[fd] = player;

        // Notify the client of their initial position and direction
        std::string initMsg = "INIT " + std::to_string(player.x) + " " + std::to_string(player.y) + " " +
                              std::to_string(player.direction.first) + " " +
                              std::to_string(player.direction.second) + "\n";
        write(fd, initMsg.c_str(), initMsg.length());
    }

    // Store the game state
    gameStates[roomId] = gameState;

    // Notify all players that the game has started
    std::string startMsg = "GAME_START\n";
    for (auto &fd : room.clients)
    {
        write(fd, startMsg.c_str(), startMsg.length());
    }
}

void GameManager::handleGameMessage(int roomId, const std::string &message)
{
    // Example: Update game state based on the action
}

void GameManager::gameLoop(int roomId)
{
    while (isGameActive(roomId))
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
    }

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
        std::string error = "Invalid command. Type 'start' to begin the game:\n";
        write(clientFd, error.c_str(), error.length());
        return;
    }

    int roomId = getClientRoomId(clientFd);
    if (roomId == -1)
    {
        std::string error = "You are not in any room.\n";
        write(clientFd, error.c_str(), error.length());
        return;
    }

    // Start the game in GameManager
    gameManager.startGame(roomId);

    // Update client's state
    clients[clientFd].state = ClientState::PLAYING_GAME;

    std::string startMsg = "Game started in Room " + std::to_string(roomId) + "\n";
    write(clientFd, startMsg.c_str(), startMsg.length());
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