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
#define MAP_WIDTH 100
#define MAP_HEIGHT 100
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
    int playerId;
    int udpPort;

    PlayerThreadArgs() : udpPort(nextPort++) {}
    static int nextPort;
};
int PlayerThreadArgs::nextPort = 14000;

// Client Information Structure
struct ClientInfo
{
    int fd;
    std::string username;
    ClientState state;
    int roomId;
    int udpFd;       // UDP socket file descriptor
    bool udpAddrSet; // Flag to indicate if UDP address is set

    ClientInfo() : fd(-1), roomId(-1), udpFd(-1), udpAddrSet(false) {}
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
    int fd;
    struct sockaddr *addr;
    socklen_t len;
    int playerId; // Newly added field for sequential player ID
    int x;
    int y;
    std::pair<int, int> direction; // (dy, dx)
    bool isAlive;

    Player() : isAlive(true) {}
    Player(int socket_fd, struct sockaddr *addr, socklen_t len, int id, int cor_x, int cor_y, std::pair<int, int> dir) : fd(socket_fd), addr(addr), len(len), playerId(id), x(cor_x), y(cor_y), direction(dir), isAlive(true) {}
};

struct GameState
{
    int roomId;
    int map[MAP_HEIGHT][MAP_WIDTH];
    std::map<int, Player> players; // Keyed by client fd
    static int nextPlayerId;       // Newly added field for tracking next player ID
    int PlayerId;
    GameState() : PlayerId(nextPlayerId++)
    {
        for (int i = 0; i < MAP_WIDTH; ++i)
        {
            for (int j = 0; j < MAP_HEIGHT; ++j)
            {
                map[i][j] = 0;
            }
        }
    }
};

int GameState::nextPlayerId(-1);

// Server Class
class Server
{
public:
    Server(int port);
    void run();
    GameManager *gameManager;
    pthread_mutex_t &getGameMutex()
    {
        gameMutex = PTHREAD_MUTEX_INITIALIZER;
        return gameMutex;
    }
    std::map<int, ClientInfo> clients; // Map to store client info keyed by file descriptor
    int listenfd;
    int maxfd;
    fd_set allset;
    pthread_mutex_t gameMutex;
    std::map<int, Room> rooms; // room
    int nextUserId;

    void setupServer(int port);
    void handleNewConnection();
    void processMessage(int clientFd);
    void sendRoomInfo(int clientFd, Server &server);
    void joinRoom(int roomId, int clientFd, Server &server);
    void broadcastMessage(const std::string &message, int exclude_fd);

private:
};

class GameManager
{
public:
    GameManager(Server *serverInstance);
    bool isGameActive(int roomId);
    void initializeGameState(int roomId, int clientFd);
    void addPlayerToGame(int roomId, int clientFd);
    bool isEnclosure(int y, int x, int roomId, int clientFd);
    std::vector<std::pair<int, int>> findInsidePoints(int roomId, int clientFd);
    void fillTerritory(const std::vector<std::pair<int, int>> &inside_points, int roomId, int clientFd);
    void handlePlayerLogic(int roomId, int clientFd, const std::string &message, struct sockaddr *cliaddr, socklen_t clilen, int udpSocket, int playerId);
    void handlePlayerDeath(int roomId, int udpSocket, int clientFd);
    Server *server;                      // Pointer back to Server
    std::map<int, GameState> gameStates; // roomId -> GameState
    std::queue<std::string> q;
    void updateAllPlayers(int roomId);
    void checkPlayerDeaths(int roomId);
    void broadcastGameState(int roomId);
    void endGame(int roomId);

private:
};
GameManager::GameManager(Server *serverInstance)
    : server(serverInstance) {}

// Implementation of RoomManager Methods
void Server::sendRoomInfo(int clientFd, Server &server)
{
    std::stringstream ssr;

    // 1. Send the number of available rooms
    ssr << rooms.size() << "\n";
    std::string roomsSize = ssr.str();

    if (write(clientFd, roomsSize.c_str(), roomsSize.length()) < 0)
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

            if (write(clientFd, info.c_str(), info.length()) < 0)
            {
                perror("write error in sendRoomInfo");
            }
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

void GameManager::initializeGameState(int roomId, int clientFd)
{
    std::cout << "Initializing game state for room " << roomId << std::endl;

    // Create a new GameState
    GameState gameState;
    gameState.roomId = roomId;

    // Store the game state
    gameStates[roomId] = gameState;
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
        pthread_mutex_unlock(&server->getGameMutex());
        return;
    }

    pthread_detach(playerThread); // Detach the thread to allow independent execution
}

void *playerThreadFunction(void *args)
{

    PlayerThreadArgs *Playerargs = (PlayerThreadArgs *)args;
    char buffer[1024];

    // 1. Create a UDP socket
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0)
    {
        perror("UDP socket creation failed");
        // Handle error appropriately (e.g., notify client, cleanup)
        return nullptr;
    }

    // 2. Bind the UDP socket to an available port
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

    // 3. Retrieve the assigned port number
    socklen_t len = sizeof(servaddr);
    if (getsockname(udpSocket, (struct sockaddr *)&servaddr, &len) == -1)
    {
        perror("getsockname failed");
        close(udpSocket);
        // Handle error appropriately
        return nullptr;
    }

    //---------------- Generate a random starting position with a minimum distance from walls
    srand(time(NULL) + udpSocket); // Seed with current time and clientFd for uniqueness
    const int MIN_DISTANCE_FROM_WALL = 20;

    int start_x = MIN_DISTANCE_FROM_WALL + rand() % (MAP_WIDTH - 2 * MIN_DISTANCE_FROM_WALL);
    int start_y = MIN_DISTANCE_FROM_WALL + rand() % (MAP_HEIGHT - 2 * MIN_DISTANCE_FROM_WALL);
    auto &gameState = Playerargs->gameManager->gameStates[Playerargs->roomId];
    // Ensure the starting position is on an empty cell
    while (gameState.map[start_y][start_x] != 0)
    {
        start_x = MIN_DISTANCE_FROM_WALL + rand() % (MAP_WIDTH - 2 * MIN_DISTANCE_FROM_WALL);
        start_y = MIN_DISTANCE_FROM_WALL + rand() % (MAP_HEIGHT - 2 * MIN_DISTANCE_FROM_WALL);
    }
    for (int dy = -2; dy <= 2; ++dy)
    {
        for (int dx = -2; dx <= 2; ++dx)
        {
            int new_x = start_x + dx;
            int new_y = start_y + dy;

            // Check map boundaries
            if (new_x >= 0 && new_x < MAP_WIDTH && new_y >= 0 && new_y < MAP_HEIGHT)
            {
                // Only set if the cell is empty
                if (gameState.map[new_y][new_x] == 0)
                {
                    gameState.map[new_y][new_x] = -gameState.nextPlayerId;
                }
            }
        }
    }
    //--------------------------------------------------------

    int assignedPort = ntohs(servaddr.sin_port);
    std::cout << "UDP connection established for Client FD " << Playerargs->clientFd << " on port " << assignedPort << "\n";
    char recv[MAXLINE];
    socklen_t clilen = sizeof(cliaddr);
    // recv ack
    recvfrom(udpSocket, recv, MAXLINE, 0, (struct sockaddr *)&cliaddr, &clilen);

    std::cout << "Player ID " << Playerargs->playerId << " (Client FD " << Playerargs->clientFd << ") added to Room " << Playerargs->roomId << "\n";
    Player player(udpSocket, (struct sockaddr *)&cliaddr, clilen, gameState.nextPlayerId, 0, 0, {0, 1});

    player.x = start_x;
    player.y = start_y;
    std::cout << player.y << " " << player.x << "\n";
    // Assign the player's ID to the map
    gameState.map[player.y][player.x] = gameState.nextPlayerId;
    gameState.players[udpSocket] = player;

    std::string ack(recv);
    std::cout << ack << "\n";
    std::string position = std::to_string(Playerargs->gameManager->gameStates[Playerargs->roomId].players[udpSocket].y) + " " + std::to_string(Playerargs->gameManager->gameStates[Playerargs->roomId].players[udpSocket].x) + "\n";
    // send position
    sendto(udpSocket, position.c_str(), position.length(), 0, (struct sockaddr *)&cliaddr, clilen);
    std::cout << "Position " << position << " sent\n";
    std::string id = std::to_string(gameState.nextPlayerId) + "\n";
    // send Id
    sendto(udpSocket, id.c_str(), id.length(), 0, (struct sockaddr *)&cliaddr, clilen);
    std::cout << "PlayerId " << id << " sent\n";
    // std::queue<std::string> q;
    while (true)
    {
        socklen_t clilen = sizeof(cliaddr);
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytesRead = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&cliaddr, &clilen);
        std::cout << buffer << "\n";
        if (bytesRead <= 0)
        {
            if (bytesRead < 0)
            {
                perror("read error in playerThreadFunction");
            }
            else
            {
                std::cout << "Client FD " << udpSocket << " disconnected.\n";
            }

            // Handle player disconnection
            Playerargs->gameManager->handlePlayerDeath(Playerargs->roomId, udpSocket, Playerargs->clientFd);
            return nullptr;
        }

        buffer[bytesRead] = '\0';
        std::string message(buffer);
        if (message == "leave")
        {
            Playerargs->gameManager->handlePlayerDeath(Playerargs->roomId, udpSocket, Playerargs->clientFd);
            return nullptr;
        }

        // Handle the received message
        Playerargs->gameManager->handlePlayerLogic(Playerargs->roomId, Playerargs->clientFd, message, (struct sockaddr *)&cliaddr, clilen, udpSocket, player.playerId);
    }
}

void GameManager::handlePlayerLogic(int roomId, int clientFd, const std::string &message, struct sockaddr *cliaddr, socklen_t clilen, int udpSocket, int playerId)
{
    pthread_mutex_lock(&server->getGameMutex());

    std::istringstream iss(message);
    int y, x;
    std::string mode;
    if (!(iss >> y >> x >> mode))
    {
        std::cerr << "Invalid message format from client FD " << clientFd << "\n";
        pthread_mutex_unlock(&server->getGameMutex());
        return;
    }

    // std::cout << "Received from FD " << udpSocket << ": " << y << " " << x << " " << mode << "\n";
    auto &gameState = gameStates[roomId];
    // auto playerIt = gameState.players.find(udpSocket);
    // if (playerIt == gameState.players.end())
    // {
    //     std::cerr << "Player not found for client FD " << udpSocket << "\n";
    //     std::string errorMsg = "ERROR Player not found\n";
    //     write(clientFd, errorMsg.c_str(), errorMsg.length());
    //     pthread_mutex_unlock(&server->getGameMutex());
    //     return;
    // }

    std::string diffMsg;
    std::map<int, std::string> yx;
    // Check if the cell is already occupied
    if (gameState.map[y][x] != 0)
    {
        int occupantPlayerId = gameState.map[y][x];

        // Check if the occupant is another player
        if (occupantPlayerId != playerId && occupantPlayerId != -playerId && occupantPlayerId > 0)
        {
            // Find the clientFd of the occupant player
            int occupantFd = -1;
            for (const auto &[fd, player] : gameState.players)
            {
                if (playerId == occupantPlayerId)
                {
                    occupantFd = fd;
                    break;
                }
            }

            if (occupantFd != -1)
            {
                std::string dieMsg = "die";
                // Send "DIE" message to the occupant player
                sendto(udpSocket, dieMsg.c_str(), dieMsg.length(), 0, cliaddr, clilen);
                // Handle occupant player's death without disconnecting TCP
                handlePlayerDeath(roomId, occupantFd, clientFd);
                std::cout << "Player ID " << occupantPlayerId << " (Client FD " << occupantFd
                          << ") has been killed by Player ID " << playerId << "\n";
            }
            else
            {
                std::cerr << "Occupant player with ID " << occupantPlayerId
                          << " not found in Room " << roomId << "\n";
            }
        }

        if (gameState.map[y][x] == -playerId)
        {
            if (isEnclosure(y, x, roomId, udpSocket))
            {
                auto inside_points = findInsidePoints(roomId, udpSocket);
                fillTerritory(inside_points, roomId, udpSocket);
                // Store inside points in yx[gameState.map[i][j]] separated by space
                for (const auto &[i, j] : inside_points)
                {
                    yx[-playerId] += std::to_string(i) + " " + std::to_string(j) + " ";
                }
            }
        }
    }
    else
    {
        // Check if the map is modified by other player threads
        yx[playerId] += std::to_string(y) + " " + std::to_string(x) + " ";
        gameState.map[y][x] = playerId;
    }
    for (const auto &[id, positions] : yx)
    {
        diffMsg = std::to_string(id) + " " + mode + " " + positions;
    }
    std::cout << "sent to client: " <<diffMsg << "\n";
    // Broadcast diffMsg and dieMsg to other connected UDP clients
    for (const auto &[otherFd, otherPlayer] : gameState.players)
    {
        std::cout << otherFd << "\n";
        if (otherFd != udpSocket)
        {
            // Send diffMsg
            sendto(otherFd, diffMsg.c_str(), diffMsg.length(), 0, gameState.players[otherFd].addr, gameState.players[otherFd].len);
            std::cout << "Sent to FD " << otherFd << ": " << diffMsg << "\n";
        }
    }

    // Update the map with the player's ID
    gameState.map[y][x] = playerId;

    pthread_mutex_unlock(&server->getGameMutex());
}

void GameManager::handlePlayerDeath(int roomId, int udpSocket, int clientFd)
{
    // Ensure thread safety
    pthread_mutex_lock(&server->getGameMutex());

    auto &gameState = gameStates[roomId];
    auto playerIt = gameState.players.find(udpSocket);
    if (playerIt == gameState.players.end())
    {
        std::cerr << "Player not found for client FD " << udpSocket << " while handling death.\n";
        pthread_mutex_unlock(&server->getGameMutex());
        return;
    }

    // Retrieve player information
    int playerId = playerIt->second.playerId;

    // Clear the player's position from the map
    gameState.map[playerIt->second.y][playerIt->second.x] = 0;
    std::string yx;
    // Iterate through the entire map to clear all cells associated with this player
    for (int y = 0; y < MAP_HEIGHT; ++y)
    {
        for (int x = 0; x < MAP_WIDTH; ++x)
        {
            if (gameState.map[y][x] == playerId || gameState.map[y][x] == -playerId)
            {
                gameState.map[y][x] = 0;
                yx += std::to_string(y) + " " + std::to_string(x) + " ";
            }
        }
    }
    std::string diffMsg = "";
    diffMsg += std::to_string(0) + " " + "NORMAL" + " " + yx;
    std::cout << diffMsg << "\n";

    // Broadcast diffMsg and dieMsg to other connected UDP clients
    for (const auto &[otherFd, otherPlayer] : gameState.players)
    {
        if (otherFd != udpSocket)
        {
            // Send diffMsg
            sendto(otherFd, diffMsg.c_str(), diffMsg.length(), 0, gameState.players[otherFd].addr, gameState.players[otherFd].len);
            std::cout << "Sent to FD " << otherFd << ": " << diffMsg << "\n";
        }
    }

    // Reset UDP connection information
    server->clients[udpSocket].udpFd = -1;
    server->clients[udpSocket].udpAddrSet = false;
    // Remove player from the game state
    gameState.players.erase(playerIt);
    // Set client state to WAITING_START
    server->clients[clientFd]
        .state = ClientState::WAITING_START;
    server->joinRoom(roomId, clientFd, *server);
    std::cout << "Player ID " << playerId << " (Client FD " << udpSocket
              << ") died.\n";

    // Close the UDP socket
    close(udpSocket);

    pthread_mutex_unlock(&server->getGameMutex());
}

bool GameManager::isEnclosure(int y, int x, int roomId, int clientFd)
{
    pthread_mutex_lock(&server->getGameMutex());
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
            if (gameStates[roomId].map[ny][nx] == gameStates[roomId].players[clientFd].playerId || gameStates[roomId].map[ny][nx] == -gameStates[roomId].players[clientFd].playerId)
            {
                visited[ny][nx] = true;
                q.push({ny, nx});
            }
        }
    }

    // If the component touches the border, it's not an enclosure
    pthread_mutex_unlock(&server->getGameMutex());
    return !touches_border;
}
std::vector<std::pair<int, int>> GameManager::findInsidePoints(int roomId, int clientFd)
{
    pthread_mutex_lock(&server->getGameMutex());
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
            if (gameStates[roomId].map[ny][nx] != gameStates[roomId].players[clientFd].playerId && gameStates[roomId].map[ny][nx] != -gameStates[roomId].players[clientFd].playerId)
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
            if (!visited[y][x] && (gameStates[roomId].map[y][x] == 0 || gameStates[roomId].map[y][x] == gameStates[clientFd].players[clientFd].playerId))
            {
                inside_points.push_back({y, x});
            }
        }
    }

    pthread_mutex_unlock(&server->getGameMutex());
    return inside_points;
}
void GameManager::fillTerritory(const std::vector<std::pair<int, int>> &inside_points, int roomId, int clientFd)
{
    pthread_mutex_lock(&server->getGameMutex());
    for (const auto &[y, x] : inside_points)
    {
        gameStates[roomId].map[y][x] = -gameStates[roomId].players[clientFd].playerId; // Mark as filled territory
    }
    pthread_mutex_unlock(&server->getGameMutex());
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

            // Update map
            gameState.map[player.y][player.x] = 0; // Clear old position
            player.x = newX;
            player.y = newY;
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
}

bool GameManager::isGameActive(int roomId)
{
    return gameStates.find(roomId) != gameStates.end();
}

void GameManager::endGame(int roomId)
{
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
    ci.fd = connfd;
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

            // Optionally, notify others in the room
            // ... (Implement if needed)
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

// Main Function
int main()
{
    Server server(SERV_PORT);
    server.run();
    return 0;
}