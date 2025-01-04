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
#include <queue>

// Constants
#define MAXCLI 10
#define MAXLINE 4096
#define SERV_PORT 12345
#define LISTENQ 10
#define MAP_WIDTH 50
#define MAP_HEIGHT 50
#define SA struct sockaddr

class GameManager;
void *playerThreadFunction(void *args);

// Define client states
enum class ClientState
{
    WAITING_USERNAME,
    ROOM_SELECTION,
    WAITING_START,
    PLAYING_GAME
};

// Structure to pass arguments to the player thread
struct PlayerThreadArgs
{
    int clientFd;
    GameManager *gameManager;
    int roomId;
    int udpPort;

    PlayerThreadArgs() : udpPort(nextPort++) {}
    static int nextPort;
};
int PlayerThreadArgs::nextPort = 14000;

// Client Information Structure
struct ClientInfo
{
    int Tcpfd;
    std::string username;
    ClientState state;
    int roomId;

    ClientInfo() : Tcpfd(-1), roomId(-1) {}
};

// Room Structure
struct Room
{
    int roomId;
    std::vector<std::string> usernames;
    std::vector<int> clientFd; // List of client FDs
};

struct Player
{
    int udpSocket;
    struct sockaddr *addr;
    socklen_t len;
    int playerId;
    static int nextPlayerId; // Newly added field for tracking next player ID
    Player() : udpSocket(-1) {};
    Player(int socket_fd, struct sockaddr *addr, socklen_t len) : udpSocket(socket_fd), addr(addr), len(len), playerId(nextPlayerId++) {}
};

struct GameState
{
    int roomId;
    int map[MAP_HEIGHT][MAP_WIDTH];
    std::map<int, Player> players; // Keyed by client fd
    std::map<int, int> IdFd;
    std::map<int, int> udpTcp;
};

int Player::nextPlayerId(1);

// Server Class
class Server
{
public:
    Server(int port);
    void run();
    GameManager *gameManager;
    std::map<int, ClientInfo> clients; // Map to store client info keyed by file descriptor
    int listenfd;
    int maxfd;
    fd_set allset;
    std::map<int, Room> rooms; // room
    int nextUserId;

    void setupServer(int port);
    void handleNewConnection();
    void processMessage(int clientFd);
    void sendRoomInfo(int clientFd, Server &server);
    void joinRoom(int roomId, int clientFd, Server &server);

private:
};

class GameManager
{
public:
    GameManager(Server *serverInstance);
    void broadcastMessage(int playerId, const std::string &message, int udpSock, int roomId);
    void broadcastMessageExceptYourself(int playerId, const std::string &message, int udpSock, int roomId);
    void initializeGameState(int roomId, int clientFd);
    void addPlayerToGame(int roomId, int clientFd);
    std::vector<std::pair<int, int>> findInsidePoints(int roomId, int clientFd);
    void fillTerritory(const std::vector<std::pair<int, int>> &inside_points, int roomId, int clientFd);
    void handlePlayerDeath(int playerId, int roomId, int udpSocket, int clientFd);

    pthread_mutex_t gameMutex = PTHREAD_MUTEX_INITIALIZER;
    Server *server;                      // Pointer back to Server
    std::map<int, GameState> gameStates; // roomId -> GameState
private:
};
GameManager::GameManager(Server *serverInstance)
    : server(serverInstance) {}

void GameManager::initializeGameState(int roomId, int clientFd)
{

    if (gameStates.find(roomId) == gameStates.end())
    {
        // Create a new GameState
        GameState gameState;
        gameState.roomId = roomId;
        for (int i = 0; i < MAP_WIDTH; ++i)
        {
            for (int j = 0; j < MAP_HEIGHT; ++j)
            {
                gameState.map[i][j] = 0;
            }
        }
        // Store the game state
        gameStates[roomId] = gameState;
        std::cout << "Initializing game state for room " << roomId << std::endl;
    }
}

void GameManager::addPlayerToGame(int roomId, int clientFd)
{
    // Prepare arguments for player thread
    PlayerThreadArgs *ptArgs = new PlayerThreadArgs();
    ptArgs->clientFd = clientFd;
    ptArgs->gameManager = this;
    ptArgs->roomId = roomId;
    std::string port = std::to_string(ptArgs->udpPort) + "\n";
    write(clientFd, port.c_str(), port.length());
    std::cout << "Port " << port << " sent\n";
    // Create a new thread for the player
    pthread_t playerThread;

    if (pthread_create(&playerThread, nullptr, playerThreadFunction, ptArgs) != 0)
    {
        delete ptArgs;
        // Handle thread creation failure (e.g., remove player from game)
        pthread_mutex_unlock(&gameMutex);
        return;
    }

    pthread_detach(playerThread); // Detach the thread to allow independent execution
}

void *playerThreadFunction(void *args)
{

    // 1.create udpSocket
    // 2.choose a random point on the map and broadcast to all player
    // 3.send initial map to that client
    // 4.recv player updated position and broadcast to other players
    PlayerThreadArgs *Playerargs = (PlayerThreadArgs *)args;

    // init gamestate
    auto &gameState = Playerargs->gameManager->gameStates[Playerargs->roomId];

    // 1.create udpSocket------------------------------------------------------
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0)
    {
        perror("UDP socket creation failed");
        // Handle error appropriately (e.g., notify client, cleanup)
        return nullptr;
    }
    struct sockaddr_in servaddr, cliaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(Playerargs->udpPort++); // Let the system choose the port
    if (bind(udpSocket, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("UDP socket bind failed");
        close(udpSocket);
        // Handle error appropriately
        return nullptr;
    }

    char recv[MAXLINE];
    socklen_t clilen = sizeof(cliaddr);
    // recv ack
    recvfrom(udpSocket, recv, MAXLINE, 0, (struct sockaddr *)&cliaddr, &clilen);
    std::string ack(recv);
    std::cout << ack << "\n";
    //---------------------------------------------------------------------

    // 2.choose a random point on the map and broadcast to all player------------------------------
    Player player(udpSocket, (struct sockaddr *)&cliaddr, clilen);
    int playerId = player.playerId;
    gameState.IdFd[playerId] = udpSocket;
    gameState.udpTcp[udpSocket] = Playerargs->clientFd;
    pthread_mutex_lock(&Playerargs->gameManager->gameMutex);

    // Insert the player
    gameState.players.emplace(udpSocket, player);

    pthread_mutex_unlock(&Playerargs->gameManager->gameMutex);

    const int MIN_DISTANCE_FROM_WALL = 20;
    int start_x = MIN_DISTANCE_FROM_WALL + rand() % (MAP_WIDTH - 2 * MIN_DISTANCE_FROM_WALL);
    int start_y = MIN_DISTANCE_FROM_WALL + rand() % (MAP_HEIGHT - 2 * MIN_DISTANCE_FROM_WALL);

    while (gameState.map[start_y][start_x] != 0)
    {
        start_x = MIN_DISTANCE_FROM_WALL + rand() % (MAP_WIDTH - 2 * MIN_DISTANCE_FROM_WALL);
        start_y = MIN_DISTANCE_FROM_WALL + rand() % (MAP_HEIGHT - 2 * MIN_DISTANCE_FROM_WALL);
    }
    std::string IdPosition = std::to_string(playerId) + " " + std::to_string(start_y) + " " + std::to_string(start_x);
    Playerargs->gameManager->broadcastMessage(playerId, IdPosition, udpSocket, Playerargs->roomId);
    srand(time(NULL) + udpSocket); // Seed with current time and clientFd for uniqueness

    for (int dy = -2; dy <= 2; ++dy)
    {
        for (int dx = -2; dx <= 2; ++dx)
        {
            int new_x = start_x + dx;
            int new_y = start_y + dy;

            // Check map boundaries
            if (new_x >= 0 && new_x < MAP_WIDTH && new_y >= 0 && new_y < MAP_HEIGHT)
            {
                gameState.map[new_y][new_x] = -player.playerId;
            }
        }
    }

    //---------------------------------------------------------------------------

    // 3. initial map--------------------------------------------------------
    std::string initMap = "";
    for (int i = 0; i < MAP_WIDTH; ++i)
    {
        for (int j = 0; j < MAP_HEIGHT; ++j)
        {
            initMap += std::to_string(gameState.map[i][j]) + " ";
        }
        initMap += "\n";
    }
    std::cout << initMap << "\n";
    sendto(udpSocket, initMap.c_str(), initMap.length(), 0, (struct sockaddr *)&cliaddr, clilen);
    //----------------------------------------------------------------------
    std::cout << "PlayerId : " << playerId << "\n";
    std::cout << start_y << " " << start_x << "\n";

    char buffer[MAXLINE];
    while (true)
    {
        socklen_t clilen = sizeof(cliaddr);
        memset(buffer, 0, sizeof(buffer));

        // Set up the file descriptor set
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(udpSocket, &readfds);

        // Set up the timeout
        struct timeval timeout;
        timeout.tv_sec = 1; // 1 seconds
        timeout.tv_usec = 0;

        // Wait for data on the socket with a 5-second timeout
        int activity = select(udpSocket + 1, &readfds, NULL, NULL, &timeout);

        if (activity < 0)
        {
            perror("select error");
            Playerargs->gameManager->handlePlayerDeath(playerId, Playerargs->roomId, udpSocket, Playerargs->clientFd);
            return nullptr;
        }
        else if (activity == 0)
        {
            // Timeout occurred, close the socket
            std::cout << "No data received in 5 seconds. Closing UDP socket." << std::endl;
            std::string heDied = std::to_string(playerId) + " -1 -1";
            Playerargs->gameManager->broadcastMessageExceptYourself(playerId, heDied, udpSocket, Playerargs->roomId);
            Playerargs->gameManager->handlePlayerDeath(playerId, Playerargs->roomId, udpSocket, Playerargs->clientFd);
            return nullptr;
        }

        if (FD_ISSET(udpSocket, &readfds))
        {
            ssize_t bytesRead = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&cliaddr, &clilen);
            if (bytesRead < 0)
            {
                perror("recvfrom failed");
                Playerargs->gameManager->handlePlayerDeath(playerId, Playerargs->roomId, udpSocket, Playerargs->clientFd);
                return nullptr;
            }
            else if (bytesRead == 0)
            {
                // Handle player disconnection
                Playerargs->gameManager->handlePlayerDeath(playerId, Playerargs->roomId, udpSocket, Playerargs->clientFd);
                return nullptr;
            }

            buffer[bytesRead] = '\0';
            std::string message(buffer);

            std::cout << udpSocket << " " << buffer << "\n";

            // Handle the received message
            Playerargs->gameManager->broadcastMessageExceptYourself(playerId, message, udpSocket, Playerargs->roomId);
            std::stringstream ss(message);
            int y, x, id;
            ss >> id >> y >> x;

            // update map
            // pthread_mutex_lock(&Playerargs->gameManager->gameMutex);
            if (gameState.map[y][x] == playerId || y <= 0 || y >= MAP_WIDTH - 1 || x <= 0 || x >= MAP_HEIGHT - 1)
            {
                Playerargs->gameManager->handlePlayerDeath(playerId, Playerargs->roomId, udpSocket, Playerargs->clientFd);
                return nullptr;
            }
            else if (gameState.map[y][x] == -playerId)
            {
                auto inside_points = Playerargs->gameManager->findInsidePoints(Playerargs->roomId, playerId);
                Playerargs->gameManager->fillTerritory(inside_points, Playerargs->roomId, playerId);
            }
            else if (gameState.map[y][x] > 0)
            {
                gameState.map[y][x] = playerId;
                int killedId = gameState.map[y][x];
                int killedUdp = gameState.IdFd[killedId];
                int killedTcp = gameState.udpTcp[killedUdp];
                gameState.IdFd.erase(killedId);
                gameState.udpTcp.erase(killedUdp);
                Playerargs->gameManager->handlePlayerDeath(killedId, Playerargs->roomId, killedUdp, killedTcp);
                return nullptr;
            }
            else
            {
                gameState.map[y][x] = playerId;
                std::string initMap = "";
                for (int i = 0; i < MAP_WIDTH; ++i)
                {
                    for (int j = 0; j < MAP_HEIGHT; ++j)
                    {
                        initMap += std::to_string(gameState.map[i][j]) + " ";
                    }
                    initMap += "\n";
                }
                std::cout << initMap << "\n";
            }
            // pthread_mutex_unlock(&Playerargs->gameManager->gameMutex);
        }
    }
}

void GameManager::broadcastMessage(int playerId, const std::string &message, int udpSock, int roomId)
{
    for (const auto &[allFd, allPlayer] : gameStates[roomId].players)
    {
        sendto(allFd, message.c_str(), message.length(), 0, gameStates[roomId].players[allFd].addr, gameStates[roomId].players[allFd].len);
        std::cout << "Sent to FD " << allFd << ": " << message << "\n";
    }
}

void GameManager::broadcastMessageExceptYourself(int playerId, const std::string &message, int udpSock, int roomId)
{
    for (const auto &[otherFd, otherPlayer] : gameStates[roomId].players)
    {
        if (otherFd != udpSock)
        {
            sendto(otherFd, message.c_str(), message.length(), 0, gameStates[roomId].players[otherFd].addr, gameStates[roomId].players[otherFd].len);
            std::cout << "Sent to FD " << otherFd << ": " << message << "\n";
        }
    }
}

void GameManager::handlePlayerDeath(int playerId, int roomId, int udpSocket, int clientFd)
{

    auto &gameState = gameStates[roomId];
    auto playerIt = gameState.players.find(udpSocket);
    if (playerIt == gameState.players.end())
    {
        std::cerr << "Player not found for udpsocket" << udpSocket << " while handling death.\n";
        pthread_mutex_unlock(&gameMutex);
        return;
    }

    // Iterate through the entire map to clear all cells associated with this player
    for (int y = 0; y < MAP_HEIGHT; ++y)
    {
        for (int x = 0; x < MAP_WIDTH; ++x)
        {
            if (gameState.map[y][x] == playerId || gameState.map[y][x] == -playerId)
            {
                gameState.map[y][x] = 0;
            }
        }
    }
    // Remove player from the game state
    gameState.players.erase(playerIt);
    // Close the UDP socket
    close(udpSocket);

    // Set client state to WAITING_START
    auto clientIt = server->clients.find(clientFd);
    if (clientIt != server->clients.end())
    {
        clientIt->second.state = ClientState::WAITING_START;
        server->joinRoom(roomId, clientFd, *server);
    }

    std::cout << "Player ID " << playerId << " (udpsocket " << udpSocket
              << ") died.\n";
}

std::vector<std::pair<int, int>> GameManager::findInsidePoints(int roomId, int playerId)
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
            if (gameStates[roomId].map[i][j] != 0 /*&& map[i][j] != id*/)
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
            if (gameStates[roomId].map[i][j] != 0 /*&& map[i][j] != id*/)
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
            if (gameStates[roomId].map[ny][nx] != playerId && gameStates[roomId].map[ny][nx] != -playerId)
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
            if (!visited[y][x] && gameStates[roomId].map[y][x] != -playerId)
            {
                inside_points.push_back({y, x});
            }
        }
    }

    return inside_points;
}
void GameManager::fillTerritory(const std::vector<std::pair<int, int>> &inside_points, int roomId, int playerId)
{
    pthread_mutex_lock(&gameMutex);
    for (const auto &[y, x] : inside_points)
    {
        gameStates[roomId].map[y][x] = -playerId; // Mark as filled territory
    }
    pthread_mutex_unlock(&gameMutex);
}

// Implementation of Server Methods
Server::Server(int port)
    : listenfd(-1), maxfd(-1),
      nextUserId(1),
      gameManager(new GameManager(this))
{
    FD_ZERO(&allset);
    setupServer(port);
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
    ci.Tcpfd = connfd;
    ci.username = "";
    ci.state = ClientState::WAITING_USERNAME;
    // Store the client information
    clients[connfd] = ci;

    // Add the new socket to the set
    FD_SET(connfd, &allset);
    if (connfd > maxfd)
        maxfd = connfd;

    std::cout << "New connection established. FD: " << connfd << std::endl;
}

// Process incoming messages based on client state
void Server::processMessage(int clientFd)
{
    char recvline[MAXLINE];
    memset(&recvline, 0, sizeof(recvline));
    ssize_t n = read(clientFd, recvline, MAXLINE - 1);
    if (n <= 0)
    {
        // Handle client disconnect
        auto clientIt = clients.find(clientFd);
        if (clientIt != clients.end())
        {
            std::cout << "Client " << clientIt->second.username << " disconnected." << std::endl;

            int roomId = clientIt->second.roomId;

            // Close and remove the client
            close(clientFd);
            FD_CLR(clientFd, &allset);
            clients.erase(clientIt);

            // If the client was in a room, remove them from the room
            if (roomId != -1)
            {
                auto roomIt = rooms.find(roomId);
                if (roomIt != rooms.end())
                {
                    // Remove clientFd from room's clientFd vector
                    roomIt->second.clientFd.erase(
                        std::remove(roomIt->second.clientFd.begin(), roomIt->second.clientFd.end(), clientFd),
                        roomIt->second.clientFd.end());

                    // Optionally remove username from room's usernames vector
                    roomIt->second.usernames.erase(
                        std::remove(roomIt->second.usernames.begin(), roomIt->second.usernames.end(), clientIt->second.username),
                        roomIt->second.usernames.end());

                    // If the room is empty, erase the room
                    if (roomIt->second.clientFd.empty())
                    {
                        rooms.erase(roomIt);
                        std::cout << "Room " << roomId << " has been cleared as it has no more clients.\n";
                    }
                }
            }
        }
        return;
    }
    std::cout << "Client sent: " << recvline << "\n";
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
    {
        clients[clientFd].username = message;
        sendRoomInfo(clientFd, *this);
        clients[clientFd].state = ClientState::ROOM_SELECTION;
        break;
    }

    case ClientState::ROOM_SELECTION:
    {
        int selectedRoomId = std::stoi(message);
        joinRoom(selectedRoomId, clientFd, *this);
        clients[clientFd].state = ClientState::WAITING_START;
        break;
    }

    case ClientState::WAITING_START:
    {
        if (message != "start")
        {
            // Update client's state back to ROOM_SELECTION
            // Retrieve the room ID the client is currently in
            int roomId = clients[clientFd].roomId;

            // Check if the room exists
            auto roomIt = rooms.find(roomId);
            if (roomIt != rooms.end())
            {
                // Remove the client from the room's client list
                roomIt->second.clientFd.erase(
                    std::remove(roomIt->second.clientFd.begin(), roomIt->second.clientFd.end(), clientFd),
                    roomIt->second.clientFd.end());

                // Remove the client's username from the room's username list
                roomIt->second.usernames.erase(
                    std::remove(roomIt->second.usernames.begin(), roomIt->second.usernames.end(), clients[clientFd].username),
                    roomIt->second.usernames.end());

                std::cout << "Client FD " << clientFd << " left Room " << roomId << std::endl;

                // If the room is now empty, erase it from the rooms map
                if (roomIt->second.clientFd.empty())
                {
                    rooms.erase(roomIt);
                    std::cout << "Room " << roomId << " has been cleared as it has no more clients.\n";
                }
                // Send room information to the client
                clients[clientFd].state = ClientState::ROOM_SELECTION;
                sendRoomInfo(clientFd, *this);
            }
        }
        else
        {
            int roomId = clients[clientFd].roomId;

            clients[clientFd].state = ClientState::PLAYING_GAME;

            gameManager->initializeGameState(roomId, clientFd);
            gameManager->addPlayerToGame(roomId, clientFd);

            std::cout << "Client FD " << clientFd << " started game in Room " << roomId << "\n";
        }
        break;
    }

    default:
        std::cerr << "Unknown state for client FD " << clientFd << std::endl;
        break;
    }
}

// Implementation of RoomManager Methods
void Server::sendRoomInfo(int clientFd, Server &server)
{
    std::stringstream roomsSize;
    // 1. Send the number of available rooms
    roomsSize << rooms.size() << "\n";
    if (write(clientFd, roomsSize.str().c_str(), roomsSize.str().length()) < 0)
    {
        perror("write error in sendRoomInfo");
    }
    // Only send room information if there are rooms available
    if (!rooms.empty())
    {
        std::stringstream ss;
        for (const auto &[roomId, room] : rooms)
        {
            ss << roomId << " " << room.clientFd.size() << "\n";
            std::string info = ss.str();
        }
        if (write(clientFd, ss.str().c_str(), ss.str().length()) < 0)
        {
            perror("write error in sendRoomInfo");
        }
    }
    std::cout << "Room info sent to Client FD " << clientFd << "\n";
}

void Server::joinRoom(int roomId, int clientFd, Server &server)
{
    auto it = rooms.find(roomId);
    if (it != rooms.end())
    {
        // Room exists. Add client to the room.
        if (std::find(it->second.clientFd.begin(), it->second.clientFd.end(), clientFd) == it->second.clientFd.end())
        {
            clients[clientFd].roomId = roomId;
            it->second.clientFd.push_back(clientFd);
            it->second.usernames.push_back(clients[clientFd].username);
            std::cout << "Client FD " << clientFd << " joined existing room " << roomId << std::endl;
        }

        // Prepare the list of other users' usernames
        std::stringstream ss;
        for (const auto &username : rooms[roomId].usernames)
        {
            ss << username << "\n";
        }

        std::string userList = ss.str();
        int userCount = rooms[roomId].usernames.size();
        std::string tmp = std::to_string(userCount) + "\n";
        write(clientFd, tmp.c_str(), tmp.length());
        ssize_t bytesSent = write(clientFd, userList.c_str(), userList.length());

        if (bytesSent < 0)
        {
            perror("write error in joinRoom");
        }
    }
    else
    {
        // Room does not exist. Create a new room with the given roomId.
        Room newRoom;

        // Add the client to the new room
        newRoom.clientFd.push_back(clientFd);
        newRoom.usernames.push_back(clients[clientFd].username);

        // Insert the new room into the rooms map
        rooms.emplace(roomId, newRoom);

        // Update the client's roomId
        clients[clientFd].roomId = roomId;
        std::string name = clients[clientFd].username + "\n";

        int shit = rooms[roomId].usernames.size();
        std::string tmp = std::to_string(shit) + "\n";
        write(clientFd, tmp.c_str(), tmp.length());

        ssize_t bytesSent = write(clientFd, name.c_str(), name.length());
        if (bytesSent < 0)
        {
            perror("write error in joinRoom");
        }
        std::cout << "Client FD " << clientFd << " created new room " << roomId << std::endl;
    }
}

// Main Function
int main()
{
    Server server(SERV_PORT);
    server.run();
    return 0;
}