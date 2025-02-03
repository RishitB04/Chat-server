#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <sstream>
#include <cstring>
#include <fstream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#define PORT 12345

using namespace std;

mutex send_mtx, clients_mtx, groups_mtx;

unordered_map<int, string> clients;               // client socket -> username
unordered_map<string, int> rclients;              // username -> client socket
unordered_map<string, string> users;              // username -> password
unordered_map<string, unordered_set<int>> groups; // group -> client sockets

// Send message to a specific client socket
void sendmess(const int &client_socket, const string &msg)
{
    lock_guard<mutex> send_lock(send_mtx); // Ensuring thread safety while sending messages
    if (send(client_socket, msg.c_str(), msg.size(), 0) == -1)
        cerr << "Error : Message Not Sent" << endl;
}

// Receive message from a client
bool recvmess(const int &client_socket, string &msg)
{
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0)
    {
        cerr << "Client disconnected." << endl;
        return false;
    }
    msg = string(buffer, bytes_received); // Store received message
    return true;
}

// Add client to the system and check if already logged in
bool add(const int &client_socket, const string &username)
{
    lock_guard<mutex> lock(clients_mtx); // Ensuring thread safety for adding clients
    if (rclients.find(username) != rclients.end())
    {
        string msg = "\033[31mAlready Logged In.\033[0m";
        sendmess(client_socket, msg);
#ifdef _WIN32
        closesocket(client_socket);
#else
        close(client_socket);
#endif
        return false;
    }
    else
        clients[client_socket] = username, rclients[username] = client_socket;
    return true;
}

// Send command list to client for invalid commands
void commands(const int &client_socket)
{
    string s = "\33[31mInvalid Command.\33[33m Commands usage:-\n/msg <username> <message>\n/broadcast <message>\n/group_msg <group_name> <message>\n/create_group <group_name>\n/leave_group <group_name>\n/join_group <group_name>\n/exit\033[0m";
    sendmess(client_socket, s);
}

// Notify all clients that a new user has joined
void joined(const int &client_socket)
{
    lock_guard<mutex> lock(clients_mtx); // Lock clients map for thread safety
    string msg = "\033[34m" + clients[client_socket] + " has joined the chat.\033[0m";
    for (auto &[id, s] : clients) // Send join notification to all other clients
    {
        if (id != client_socket)
        {
            sendmess(id, msg);
        }
    }
}

// Split a string into arguments based on spaces
void arguments(const string &s, vector<string> &arg)
{
    stringstream ss(s);
    string word;
    while (ss >> word)
    {
        arg.push_back(word);
    }
}

// Handle direct message command
void message(const int &client_socket, const vector<string> &arg)
{
    string s;
    if (arg.size() < 3)
    {
        s = "\033[31mInvalid Command.\033[33m Command usage:-\n/msg <username> <message>\033[0m";
        sendmess(client_socket, s);
        return;
    }
    lock_guard<mutex> lock(clients_mtx);
    if (users.find(arg[1]) == users.end())
    {
        s = "\033[33m" + arg[1] + " doesn't exist.\033[0m";
        sendmess(client_socket, s);
        return;
    }
    if (rclients.find(arg[1]) == rclients.end())
    {
        s = "\033[33m" + arg[1] + " is not online.\033[0m";
        sendmess(client_socket, s);
        return;
    }
    s = "[" + clients[client_socket] + "] :";
    for (int i = 2; i < arg.size(); i++)
    {
        s += " " + arg[i];
    }

    sendmess(rclients[arg[1]], s); // Send message to the target user
}

// Handle broadcast message command to all clients
void broadcast(const int &client_socket, const vector<string> &arg)
{
    string s;
    if (arg.size() < 2)
    {
        s = "\033[31mInvalid Command.\033[33m Command usage:-\n/broadcast <message>\033[0m";
        sendmess(client_socket, s);
        return;
    }
    s = "[" + clients[client_socket] + "] :";
    for (int i = 1; i < arg.size(); i++)
    {
        s += " " + arg[i];
    }
    lock_guard<mutex> lock(clients_mtx); // Lock clients map for thread safety
    for (auto &[id, s] : clients)        // Send broadcast message to all clients except the sender
    {
        if (id != client_socket)
        {
            sendmess(id, s);
        }
    }
}

// Handle group message command
void group_msg(const int &client_socket, const vector<string> &arg)
{
    string s;
    if (arg.size() < 3)
    {
        s = "\033[31mInvalid Command.\033[33m Command should be of the following format:-\n/group_msg <group_name> <message>\033[0m";
        sendmess(client_socket, s);
        return;
    }
    lock_guard<mutex> groups_lock(groups_mtx); // Lock groups map for thread safety
    if (groups.find(arg[1]) == groups.end())
    {
        s = "\033[33mNo group named " + arg[1] + " exists.\033[0m";
        sendmess(client_socket, s);
        return;
    }
    s = "[Group " + arg[1] + "] :";
    for (int i = 2; i < arg.size(); i++)
    {
        s += " " + arg[i];
    }
    for (auto &id : groups[arg[1]]) // Send message to all group members
    {
        lock_guard<mutex> lock(clients_mtx);
        if (id != client_socket && clients.find(id) != clients.end())
        {
            sendmess(id, s);
        }
    }
}

// Create a new group and add the client to it
void create_grp(const int &client_socket, const vector<string> &arg)
{
    string s;
    if (arg.size() != 2)
    {
        s = "\033[31mInvalid Command.\033[33m Command usage:-\n/create_group <group_name>\033[0m";
        sendmess(client_socket, s);
        return;
    }
    lock_guard<mutex> groups_lock(groups_mtx); // Lock groups map for thread safety
    if (groups.find(arg[1]) == groups.end())
    {
        groups[arg[1]].insert(client_socket); // Add client to the new group
        s = "\033[32mGroup " + arg[1] + " created.\033[0m";
        sendmess(client_socket, s);
        return;
    }
    s = "\033[33mGroup " + arg[1] + " already exists.\033[0m";
    sendmess(client_socket, s);
}

// Join an existing group
void join_grp(const int &client_socket, const vector<string> &arg)
{
    string s;
    if (arg.size() != 2)
    {
        s = "\033[31mInvalid Command.\033[33m Command usage:-\n/join_group <group_name>\033[0m";
        sendmess(client_socket, s);
        return;
    }
    lock_guard<mutex> groups_lock(groups_mtx); // Lock groups map for thread safety
    if (groups.find(arg[1]) == groups.end())
    {
        s = "\033[33mNo group named " + arg[1] + " exists.\033[0m";
        sendmess(client_socket, s);
        return;
    }
    if (groups[arg[1]].find(client_socket) != groups[arg[1]].end())
    {
        s = "\033[33mYou are already part of the group " + arg[1] + ".\033[0m";
        sendmess(client_socket, s);
        return;
    }
    groups[arg[1]].insert(client_socket); // Add client to the group
    s = "\033[32mYou joined the group " + arg[1] + ".\033[0m";
    sendmess(client_socket, s);
}

// Leave an existing group
void leave_grp(const int &client_socket, const vector<string> &arg)
{
    string s;
    if (arg.size() != 2)
    {
        s = "\033[31mInvalid Command.\033[33m Command usage:-\n/leave_group <group_name>\033[0m";
        sendmess(client_socket, s);
        return;
    }
    lock_guard<mutex> groups_lock(groups_mtx); // Lock groups map for thread safety
    if (groups.find(arg[1]) == groups.end())
    {
        s = "\033[33mNo group named " + arg[1] + " exists.\033[0m";
        sendmess(client_socket, s);
        return;
    }
    groups[arg[1]].erase(client_socket); // Remove client from the group
}

// Handle the client's interaction with the server
void interact_client(const int &client_socket)
{
    string msg;
    joined(client_socket); // Notify others when a client joins
    while (true)
    {
        vector<string> arg;
        if (!recvmess(client_socket, msg)) // Receive message from client
            break;
        arguments(msg, arg); // Parse the command
        if (arg[0] == "/msg")
        {
            message(client_socket, arg); // Handle direct message
        }
        else if (arg[0] == "/broadcast")
        {
            broadcast(client_socket, arg); // Handle broadcast message
        }
        else if (arg[0] == "/group_msg")
        {
            group_msg(client_socket, arg); // Handle group message
        }
        else if (arg[0] == "/leave_group")
        {
            leave_grp(client_socket, arg); // Handle leave group command
        }
        else if (arg[0] == "/create_group")
        {
            create_grp(client_socket, arg); // Handle create group command
        }
        else if (arg[0] == "/join_group")
        {
            join_grp(client_socket, arg); // Handle join group command
        }
        else if (msg == "/exit") // Handle exit command
        {
            break;
        }
        else
            commands(client_socket); // Handle invalid command
    }

    lock_guard<mutex> lock(clients_mtx);
    rclients.erase(clients[client_socket]); // Remove the client from active clients
    clients.erase(client_socket);           // Erase the client from the clients map
#ifdef _WIN32
    closesocket(client_socket); // Close the socket (Windows)
#else
    close(client_socket); // Close the socket (Linux/Mac)
#endif
}

// Handle client authorization (login process)
void authorization(int client_socket)
{
    string user = "Enter Username : ";
    string pass = "Enter Password : ";
    string s;
    sendmess(client_socket, user); // Prompt for username
    if (!recvmess(client_socket, user))
    {
#ifdef _WIN32
        closesocket(client_socket);
#else
        close(client_socket);
#endif
        return;
    }
    sendmess(client_socket, pass); // Prompt for password
    if (!recvmess(client_socket, pass))
    {
#ifdef _WIN32
        closesocket(client_socket);
#else
        close(client_socket);
#endif
        return;
    }
    if (users.find(user) == users.end() || pass != users[user]) // Validate user and password
    {
        s = "\033[31mAuthentication Failed.\033[0m";
        sendmess(client_socket, s);
#ifdef _WIN32
        closesocket(client_socket);
#else
        close(client_socket);
#endif
        return;
    }
    if (add(client_socket, user)) // Add the client if authentication is successful
    {
        s = "\033[32mWelcome to the chat server!\033[0m";
        sendmess(client_socket, s);
        interact_client(client_socket); // Start client interaction
    }
}

int main()
{
    // loading user database from file
    string line;
    ifstream fin;
    fin.open("users.txt");
    if (!fin.is_open())
    {
        cerr << "Error: Unable to open users.txt file." << endl;
        return -1;
    }
    while (getline(fin, line))
    {
        int i = line.find(':');
        users[line.substr(0, i)] = line.substr(i + 1); // Parse user data
    }
    cout << "k\n";
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData); // Initialize WinSock
    if (result != 0)
    {
        perror("Error : WSAstartup Failed");
        return 1;
    }
#endif

    cout << result << "\n";
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

#ifdef _WIN32
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) // Create socket (Windows)
    {
        perror("Error : Socket Failed");
        WSACleanup();
        return -1;
    }
#else
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) // Create socket (Linux/Mac)
    {
        perror("Error : Socket Failed");
        return -1;
    }
#endif

    cout << server_fd << "\n";
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT); // Set the server address and port

    if (bind(server_fd, (struct sockaddr *)&address, addrlen) < 0) // Bind the server socket
    {
        perror("Error: Bind failed");
#ifdef _WIN32
        closesocket(server_fd);
        WSACleanup();
#else
        close(server_fd);
#endif
        return 1;
    }

    cout << "k\n";
    if (listen(server_fd, SOMAXCONN) < 0) // Listen for incoming connections
    {
        perror("Error : Listen Failed");
#ifdef _WIN32
        closesocket(server_fd);
        WSACleanup();
#else
        close(server_fd);
#endif
        return 1;
    }
    cout << "k\n";

    while (true)
    {
        cout << "k\n";
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) // Accept incoming connections
        {
            perror("Accept");
            continue;
        }
        thread t(authorization, new_socket); // Handle client authorization in a separate thread
        if (t.joinable())
            t.detach();
        cout << new_socket << "\n";
    }
    cout << "terminated\n";

#ifdef _WIN32
    closesocket(server_fd);
    WSACleanup(); // Clean up WinSock
#else
    close(server_fd); // Close server socket
#endif
    return 0;
}