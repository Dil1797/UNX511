//Logger.cpp - Logging system for the client

#include "Logger.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

const int CLIENT_LISTEN_PORT = 8081;
const int SERVER_PORT = 8080;
const int BUF_LEN = 1024;

// Global variables for the logger
int log_level = DEBUG;
int client_fd;
struct sockaddr_in server_addr;
std::atomic<bool> listen_flag(false);
std::thread listen_thread;

// Thread function to listen for commands from the server
void listen_for_commands() {
    char buffer[BUF_LEN];
    struct sockaddr_in server_addr_listen;
    socklen_t addr_len = sizeof(server_addr_listen);

    // Set a timeout for the socket to make recvfrom non-blocking
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    while (listen_flag) {
        int len = recvfrom(client_fd, buffer, BUF_LEN, 0, (struct sockaddr*)&server_addr_listen, &addr_len);
        if (len > 0) {
            std::string command(buffer, len);
            if (command.find("Set Log Level=") != std::string::npos) {
                try {
                    int new_level = std::stoi(command.substr(command.find("=") + 1));
                    if (new_level >= DEBUG && new_level <= CRITICAL) {
                        log_level = new_level;
                        std::cout << "Client received new log level: " << log_level << std::endl;
                    }
                } catch (...) {
                    // Ignore malformed commands
                }
            }
        }
        memset(buffer, 0, BUF_LEN);
    }
}

void InitializeLog() {
    // Create a UDP socket for the client to listen on
    if ((client_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket failed");
        return;
    }

    // FIX: Set the SO_REUSEADDR option to prevent "Bind failed" errors
    int optval = 1;
    if (setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("setsockopt failed");
        return;
    }

    struct sockaddr_in client_addr_listen;
    memset(&client_addr_listen, 0, sizeof(client_addr_listen));
    client_addr_listen.sin_family = AF_INET;
    client_addr_listen.sin_addr.s_addr = INADDR_ANY;
    client_addr_listen.sin_port = htons(CLIENT_LISTEN_PORT);

    if (bind(client_fd, (const struct sockaddr*)&client_addr_listen, sizeof(client_addr_listen)) < 0) {
        perror("Logger: Bind failed for client listener");
        close(client_fd);
        client_fd = -1;
        return;
    }

    // Set up server address for sending logs
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(client_fd);
        client_fd = -1;
        return;
    }
    
    listen_flag = true;
    listen_thread = std::thread(listen_for_commands);
}

void SetLogLevel(LOG_LEVEL level) {
    log_level = level;
}

void Log(LOG_LEVEL level, const std::string& file, const std::string& function, int line, const std::string& message) {
    if (level < log_level) {
        return;
    }
    
    // Get current time
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");

    std::string level_str;
    switch(level) {
        case DEBUG: level_str = "DEBUG"; break;
        case WARNING: level_str = "WARNING"; break;
        case ERROR: level_str = "ERROR"; break;
        case CRITICAL: level_str = "CRITICAL"; break;
    }

    std::string log_message_str = ss.str() + " " + level_str + " " + file + ":" + function + ":" + std::to_string(line) + " " + message;
    
    if (client_fd != -1) {
        sendto(client_fd, log_message_str.c_str(), log_message_str.length(), 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    }
}

void ExitLog() {
    listen_flag = false;
    if (listen_thread.joinable()) {
        listen_thread.join();
    }
    if (client_fd != -1) {
        close(client_fd);
    }
}

