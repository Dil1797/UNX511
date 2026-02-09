#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <atomic>
#include <arpa/inet.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

const int SERVER_PORT = 8080;
const int CLIENT_LISTEN_PORT = 8081;
const int BUF_LEN = 1024;
std::atomic<bool> shutdown_flag(false);
std::thread receive_thread;

// Server now maintains its own log level for filtering the dump output
enum LOG_LEVEL { DEBUG, WARNING, ERROR, CRITICAL };
LOG_LEVEL current_server_log_level = DEBUG;

// Function to safely write a message to the log file
void log_message(const std::string& message) {
    std::ofstream log_file("server_log.txt", std::ios::app);
    if (log_file.is_open()) {
        log_file << message << std::endl << std::endl;
        log_file.close();
    } else {
        std::cerr << "Error: Unable to open log file." << std::endl;
    }
}

// Function to parse the log level from a log message string
LOG_LEVEL get_log_level_from_string(const std::string& log_line) {
    if (log_line.find("CRITICAL") != std::string::npos) return CRITICAL;
    if (log_line.find("ERROR") != std::string::npos) return ERROR;
    if (log_line.find("WARNING") != std::string::npos) return WARNING;
    return DEBUG;
}

// dump_log_file now takes a boolean to switch between exact and cumulative filtering
void dump_log_file(bool exact_match) {
    std::cout << "---- server_log.txt (filtered " << (exact_match ? "exactly" : "cumulatively") << " for level " << current_server_log_level << ") ----" << std::endl;
    std::ifstream log_file("server_log.txt");
    if (log_file.is_open()) {
        std::string line;
        while (getline(log_file, line)) {
            // Check if the line has a log level and if it meets the filtering criteria
            LOG_LEVEL line_level = get_log_level_from_string(line);
            if (exact_match) {
                if (line_level == current_server_log_level) {
                    std::cout << line << std::endl;
                }
            } else {
                if (line_level >= current_server_log_level) {
                    std::cout << line << std::endl;
                }
            }
        }
        log_file.close();
    } else {
        std::cerr << "Error: Unable to open log file." << std::endl;
    }
    std::cout << "---- end of server_log.txt ----" << std::endl;
}

// Thread function to receive log messages via UDP
void receive_logs(int server_fd) {
    char buffer[BUF_LEN];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    // Set a timeout for the socket to make recvfrom non-blocking
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    while (!shutdown_flag) {
        int len = recvfrom(server_fd, buffer, BUF_LEN, 0, (struct sockaddr *)&client_addr, &addr_len);
        if (len > 0) {
            log_message(std::string(buffer, len));
        }
        memset(buffer, 0, BUF_LEN);
    }
}

int main() {
    // Clear the log file on startup
    std::ofstream("server_log.txt", std::ios::trunc).close();

    int server_fd;
    struct sockaddr_in server_addr;
    
    // Create a UDP socket for the server to listen on
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Start the thread to receive incoming log messages
    receive_thread = std::thread(receive_logs, server_fd);

    char choice;
    std::string input;
    while (!shutdown_flag) {
        std::cout << "\n==== Log Server Menu ====" << std::endl;
        std::cout << "1. Set the log level (send 'Set Log Level=<n>' to client)" << std::endl;
        std::cout << "2. Dump the server log file here" << std::endl;
        std::cout << "0. Shut down" << std::endl;
        std::cout << "Enter choice: ";
        std::getline(std::cin, input);
        if (input.length() > 0) {
            choice = input[0];
        } else {
            choice = ' ';
        }

        switch (choice) {
            case '1': {
                std::cout << "Enter log level number (0=DEBUG,1=WARNING,2=ERROR,3=CRITICAL): ";
                std::string level_str;
                std::getline(std::cin, level_str);
                if (!level_str.empty()) {
                    int level = std::stoi(level_str);
                    if (level >= 0 && level <= 3) {
                        std::string command = "Set Log Level=" + std::to_string(level);
                        
                        // Update the server's log level filter
                        current_server_log_level = static_cast<LOG_LEVEL>(level);
                        
                        int sock = 0;
                        struct sockaddr_in client_addr; 
                        
                        if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                            std::cerr << "\n Socket creation error \n";
                            break;
                        }
                        client_addr.sin_family = AF_INET;
                        client_addr.sin_port = htons(CLIENT_LISTEN_PORT);
                        
                        if (inet_pton(AF_INET, "127.0.0.1", &client_addr.sin_addr) <= 0) {
                            std::cerr << "\nInvalid address/ Address not supported \n";
                            break;
                        }
                        
                        sendto(sock, command.c_str(), command.length(), 0, (const struct sockaddr *)&client_addr, sizeof(client_addr));
                        std::cout << "Sent command to client. Waiting for new logs to arrive..." << std::endl;
                        close(sock);

                        // Wait for a few seconds to let the client send new logs
                        std::this_thread::sleep_for(std::chrono::seconds(5));
                        
                        // Dump the log file to show the new messages, using exact match
                        dump_log_file(true);
                    } else {
                        std::cerr << "Invalid log level." << std::endl;
                    }
                }
                std::cout << "Press ENTER to continue..." << std::endl;
                std::cin.ignore();
                break;
            }
            case '2': {
                // Dump the log file with cumulative filtering
                dump_log_file(false);
                std::cout << "Press ENTER to continue..." << std::endl;
                std::cin.ignore();
                break;
            }
            case '0':
                shutdown_flag = true;
                break;
            default:
                std::cout << "Invalid choice. Please try again." << std::endl;
                break;
        }
    }

    shutdown_flag = true;
    if (receive_thread.joinable()) {
        receive_thread.join();
    }
    close(server_fd);
    return 0;
}

