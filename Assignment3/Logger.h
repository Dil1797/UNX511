//Logger.h - Logging system for the client
//
#pragma once

#include <string>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <atomic>

// This is the new definition of the log levels.
enum LOG_LEVEL { DEBUG, WARNING, ERROR, CRITICAL };

// Global variables for the logger
extern int log_level;
extern int client_fd;
extern struct sockaddr_in server_addr;
extern std::atomic<bool> listen_flag;
extern std::thread listen_thread;

// Function prototypes
void InitializeLog();
void SetLogLevel(LOG_LEVEL level);
void Log(LOG_LEVEL level, const std::string& file, const std::string& function, int line, const std::string& message);
void ExitLog();

