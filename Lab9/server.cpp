#include <iostream>
#include <string>
#include <queue>
#include <vector>
#include <cstring>
#include <csignal>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

bool is_running = true;
queue<string> message_queue;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
vector<pthread_t> threads;
vector<int> client_fds;

// Function to handle Ctrl+C (SIGINT)
void signal_handler(int signum) {
    cout << "\nSIGINT received. Setting is_running to false." << endl;
    is_running = false;
}

// This function runs in a thread for each client
void* receive_thread(void* arg) {
    // Correctly get the client file descriptor and clean up the allocated memory
    int* client_fd_ptr = (int*)arg;
    int client_fd = *client_fd_ptr;
    delete client_fd_ptr;

    char buffer[4096];
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    // Set socket option to use the timeout
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    cout << "Receive thread started for client_fd " << client_fd << endl;

    while (is_running) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = read(client_fd, buffer, sizeof(buffer));

        if (bytes > 0) {
            // Data was received
            cout << "Received " << bytes << " bytes from client_fd " << client_fd << endl;
            pthread_mutex_lock(&queue_mutex);
            message_queue.push(string(buffer));
            pthread_mutex_unlock(&queue_mutex);
        } else if (bytes == 0) {
            // Client closed connection gracefully
            cout << "Client " << client_fd << " closed connection gracefully. Exiting thread." << endl;
            // The thread will now exit its while loop and then terminate
            break;
        } else {
            // An error occurred or timeout
            if (is_running) {
                // Read timed out, check the is_running flag again
                // This is the expected behavior for non-data-sending periods
            }
        }
    }

    cout << "Receive thread for client_fd " << client_fd << " is exiting." << endl;
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: ./server <port>" << endl;
        return 1;
    }

    signal(SIGINT, signal_handler);

    int port = atoi(argv[1]);
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    // Set the socket to be non-blocking
    fcntl(server_fd, F_SETFL, O_NONBLOCK);

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Failed to bind to port " << port << endl;
        return 1;
    }

    listen(server_fd, 5);
    cout << "Server is running on port " << port << endl;

    while (is_running) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        
        if (client_fd >= 0) {
            cout << "Client connected with file descriptor: " << client_fd << endl;
            client_fds.push_back(client_fd);

            pthread_t tid;
            // Allocate memory for the file descriptor and pass its pointer to the thread
            int* client_fd_ptr = new int(client_fds.back());
            pthread_create(&tid, NULL, receive_thread, client_fd_ptr);
            threads.push_back(tid);
        }

        pthread_mutex_lock(&queue_mutex);
        while (!message_queue.empty()) {
            cout << message_queue.front();
            message_queue.pop();
        }
        pthread_mutex_unlock(&queue_mutex);

        sleep(1);
    }

    // Server shutdown sequence
    cout << "Server is stopping..." << endl;
    
    for (int fd : client_fds) {
        cout << "Sending 'Quit' to client fd: " << fd << endl;
        write(fd, "Quit", 4);
        close(fd);
    }

    cout << "Joining all receive threads..." << endl;
    for (pthread_t tid : threads) {
        pthread_join(tid, NULL);
    }
    cout << "All threads joined successfully." << endl;

    close(server_fd);
    cout << "Server socket closed. Exiting." << endl;

    return 0;
}
