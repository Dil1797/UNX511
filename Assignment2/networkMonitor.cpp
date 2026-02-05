#include <iostream>
#include <vector>
#include <string>
#include <map>          // To store client FD to interface name mapping
#include <sstream>      // For parsing user input
#include <algorithm>    // For std::remove (if needed, or use vector erase)
#include <limits>       // For std::numeric_limits

// POSIX/Linux specific headers
#include <unistd.h>     // For fork(), execve(), close()
#include <sys/socket.h> // For socket(), bind(), listen(), accept(), send(), recv()
#include <sys/un.h>     // For sockaddr_un (Unix domain sockets)
#include <sys/wait.h>   // For waitpid() (to reap zombie children)
#include <sys/select.h> // For select(), FD_SET, FD_ZERO, etc.
#include <signal.h>     // For signal()
#include <cstdio>       // For remove() (unlink)
#include <cstring>      // For memset, strncpy
#include <errno.h>      // For errno and perror

using namespace std; // Added as requested

// Define common socket path and message buffer size
#define SOCKET_PATH "/tmp/network_monitor_socket"
#define BUFFER_SIZE 256

// Global flag for graceful shutdown
volatile sig_atomic_t running = 1;

// Master listening socket file descriptor
int master_socket_fd = -1;

// Map to store connected intfMonitor FDs and their associated interface names
// Key: socket file descriptor, Value: interface name
map<int, string> client_fds;

// Function prototypes
void cleanup_sockets();
void sig_handler(int signo);
void handle_client_message(int client_fd);
void send_message(int sock_fd, const string& message);

// --- Signal Handler ---
void sig_handler(int signo) {
    if (signo == SIGINT) {
        running = 0;
#ifdef DEBUG
        cerr << "\nDEBUG: networkMonitor received SIGINT. Initiating graceful shutdown." << endl;
#endif
        // Close master socket immediately to unblock accept() if it's waiting
        if (master_socket_fd != -1) {
            close(master_socket_fd);
            master_socket_fd = -1;
        }
    }
}

// --- Helper to send message over socket ---
void send_message(int sock_fd, const string& message) {
    if (sock_fd != -1) {
        if (send(sock_fd, message.c_str(), message.length(), 0) == -1) {
            perror("networkMonitor send message");
            // Consider handling broken pipes/connections here (e.g., remove client_fd)
            // Note: If a client has already disconnected (e.g., due to its own SIGINT),
            // this send will fail with EPIPE. The recv() on this client_fd later
            // will detect the disconnection.
        }
    }
}

// --- Handles messages from an intfMonitor client ---
void handle_client_message(int client_fd) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);

    if (bytes_received > 0) {
        buffer[bytes_received] = '\0'; // Null-terminate the received string
        string message(buffer);
        string iface_name = client_fds[client_fd];

#ifdef DEBUG
        cout << "DEBUG: NM received '" << message << "' from " << iface_name << " (FD: " << client_fd << ")" << endl;
#endif

        if (message == "Ready") {
            // intfMonitor is ready, instruct it to monitor
#ifdef DEBUG
            cout << "DEBUG: Sending 'Monitor' to " << iface_name << endl;
#endif
            send_message(client_fd, "Monitor");
        }
        else if (message == "Monitoring") {
            // intfMonitor confirmed it started monitoring (optional confirmation)
#ifdef DEBUG
            cout << "DEBUG: " << iface_name << " confirmed 'Monitoring'." << endl;
#endif
        }
        else if (message == "Link Down") {
            // intfMonitor reported link down, instruct it to set link up
            cout << "ALERT: " << iface_name << " reported 'Link Down'. Sending 'Set Link Up'." << endl; // Keep this always on
            send_message(client_fd, "Set Link Up");
        }
        else if (message == "Done") {
            // intfMonitor is shutting down gracefully
#ifdef DEBUG
            cout << "DEBUG: " << iface_name << " sent 'Done'. Closing connection." << endl;
#endif
            close(client_fd);
            client_fds.erase(client_fd); // Remove from map
        }
        else {
            // This is a warning, typically kept on regardless of DEBUG flag
            cerr << "WARNING: Unknown message from " << iface_name << ": " << message << endl;
        }
    }
    else if (bytes_received == 0) {
        // Connection closed by client
#ifdef DEBUG
        cout << "DEBUG: Client " << client_fds[client_fd] << " (FD: " << client_fd << ") disconnected." << endl;
#endif
        close(client_fd);
        client_fds.erase(client_fd); // Remove from map
    }
    else {
        // Error on recv, but check if it's due to SIGINT interrupting
        if (errno == EINTR) {
#ifdef DEBUG
            cerr << "DEBUG: recv interrupted by signal (likely SIGINT)." << endl;
#endif
        }
        else {
            perror("networkMonitor recv client message");
            // This is an error, typically kept on regardless of DEBUG flag
            cerr << "ERROR: Disconnecting client " << client_fds[client_fd] << " due to recv error." << endl;
            close(client_fd);
            client_fds.erase(client_fd);
        }
    }
}

// --- Cleanup function for sockets ---
void cleanup_sockets() {
#ifdef DEBUG
    cout << "DEBUG: Cleaning up networkMonitor sockets and child processes." << endl;
#endif

    // Send "Shut Down" message to all connected intfMonitors
    for (auto const& pair : client_fds) { // Use 'pair' to avoid issues with map changes during iteration
        int fd = pair.first;
        const string& name = pair.second;
#ifdef DEBUG
        cout << "DEBUG: Sending 'Shut Down' to " << name << " (FD: " << fd << ")" << endl;
#endif
        send_message(fd, "Shut Down");
        close(fd); // Close the client socket
    }
    client_fds.clear(); // Clear the map after iterating and closing

    // Close the master listening socket if it's still open
    if (master_socket_fd != -1) {
        close(master_socket_fd);
        master_socket_fd = -1;
    }

    // Remove the socket file
    if (remove(SOCKET_PATH) == -1 && errno != ENOENT) {
        perror("remove socket file");
    }

    // Wait for any child processes to exit (to prevent zombies)
    // Non-blocking waitpid, in case some intfMonitors are still busy
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
#ifdef DEBUG
        cout << "DEBUG: Reaped child process (PID: " << pid << ")." << endl;
#endif
    }
}

int main() {
    // Register SIGINT handler
    signal(SIGINT, sig_handler);

    vector<string> interface_names;
    int num_interfaces;

    // 1. Query user for interfaces
    cout << "Enter the number of network interfaces to monitor: ";
    cin >> num_interfaces;

    // Clear the input buffer after reading int
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    for (int i = 0; i < num_interfaces; ++i) {
        string name;
        cout << "Enter name for interface " << (i + 1) << ": ";
        getline(cin, name);
        interface_names.push_back(name);
    }

    // Check if any interfaces were entered
    if (interface_names.empty()) {
        cerr << "No interfaces specified. Exiting." << endl;
        return 1;
    }

    // 2. Create and bind master socket
    master_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (master_socket_fd == -1) {
        perror("networkMonitor master socket");
        return 1;
    }

    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    // Ensure the socket file doesn't exist from a previous run
    remove(SOCKET_PATH);

    if (bind(master_socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("networkMonitor bind");
        close(master_socket_fd);
        return 1;
    }

    // 3. Listen for incoming connections
    if (listen(master_socket_fd, 5) == -1) { // 5 is backlog queue size
        perror("networkMonitor listen");
        close(master_socket_fd);
        remove(SOCKET_PATH);
        return 1;
    }
#ifdef DEBUG
    cout << "DEBUG: networkMonitor listening on " << SOCKET_PATH << endl;
#endif

    // 4. Fork and Exec intfMonitors
    for (const string& iface : interface_names) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("networkMonitor fork");
            // Handle error: perhaps log and continue or exit
            continue;
        }
        else if (pid == 0) {
            // Child process
            // Close master socket in child, it won't be used
            close(master_socket_fd);

            // Prepare arguments for execve
            // char* const argv[] = { (char*)"./intfMonitor", (char*)iface.c_str(), nullptr };
            // For simplicity, let's just use the executable path directly
            char executable_path[] = "./intfMonitor"; // Must be writable for execve
            char* args[3];
            args[0] = executable_path;
            args[1] = (char*)iface.c_str(); // Cast to char* is often needed for execve
            args[2] = nullptr;

#ifdef DEBUG
            cout << "DEBUG: Child for " << iface << " attempting execve." << endl;
#endif
            // No need for envp, just pass nullptr
            execve(executable_path, args, nullptr);

            // If execve returns, it means an error occurred
            perror("networkMonitor execve intfMonitor");
            exit(1); // Child process exits on execve failure
        }
        // Parent process continues loop
    }

    // 5. Main select() loop to manage connections
    fd_set read_fds;
    int max_fd;

    while (running) {
        FD_ZERO(&read_fds);
        max_fd = master_socket_fd; // Start with master socket FD

        FD_SET(master_socket_fd, &read_fds); // Add master socket to set

        // Add all connected client sockets to the set
        for (auto const& pair : client_fds) { // Use 'pair' to avoid issues with map changes during iteration
            int fd = pair.first;
            FD_SET(fd, &read_fds);
            if (fd > max_fd) {
                max_fd = fd;
            }
        }

        // Use a timeout for select to allow for clean shutdown and polling
        struct timeval timeout;
        timeout.tv_sec = 1; // Check for connections/messages every second
        timeout.tv_usec = 0;

        int activity = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);

        if (activity < 0 && errno != EINTR) { // EINTR means interrupted by signal (like SIGINT)
            perror("networkMonitor select");
            running = 0; // Critical error, force shutdown
        }

        if (activity < 0 && errno == EINTR) {
            // SIGINT occurred, select was interrupted. Loop will re-evaluate 'running' flag.
            continue;
        }

        // If master socket has activity (new connection)
        if (FD_ISSET(master_socket_fd, &read_fds)) {
            struct sockaddr_un client_addr;
            socklen_t client_len = sizeof(client_addr);
            int new_client_fd = accept(master_socket_fd, (struct sockaddr*)&client_addr, &client_len);

            if (new_client_fd == -1) {
                if (errno == EINTR) { // Could be interrupted by SIGINT during accept
#ifdef DEBUG
                    cerr << "DEBUG: accept interrupted by signal." << endl;
#endif
                    continue;
                }
                perror("networkMonitor accept");
                // Don't necessarily shut down, just log and continue
            }
            else {
                // This is a new client (intfMonitor)
                // We need to associate this new_client_fd with an interface name.
                // The assignment implies a synchronous setup: NM forks, then IM connects.
                // So, the order of connection should match the order of forking.
                // A more robust solution might involve the IM sending its name after connecting.
                // For this example, we'll assign them sequentially. In a real system, the intfMonitor
                // would send its interface name as the first message.
                // For now, let's assume we match FDs to interfaces by the order they connect.
                // This is a simplification that might break if connections are not in order.

                // Temporarily, we'll try to guess based on pending connections.
                // A proper solution involves the client sending its identity.
                // Since this is a direct socket per intf, they connect one-by-one.
                // We'll pick the first available interface name not yet associated.
                bool assigned = false;
                for (const string& name : interface_names) {
                    bool name_in_use = false;
                    for (auto const& pair_in_map : client_fds) {
                        const string& name_in_map = pair_in_map.second;
                        if (name_in_map == name) {
                            name_in_use = true;
                            break;
                        }
                    }
                    if (!name_in_use) {
                        client_fds[new_client_fd] = name;
#ifdef DEBUG
                        cout << "DEBUG: New connection from intfMonitor for " << name << " (FD: " << new_client_fd << ")" << endl;
#endif
                        assigned = true;
                        break;
                    }
                }
                if (!assigned) {
                    // This is a warning, typically kept on regardless of DEBUG flag
                    cerr << "WARNING: New intfMonitor connected, but no unassigned interface name available. Closing connection." << endl;
                    close(new_client_fd);
                }
            }
        }

        // Check for activity on existing client sockets
        // Iterate over a copy or use a safe iteration pattern if removing elements
        vector<int> fds_to_check;
        for (auto const& pair : client_fds) {
            fds_to_check.push_back(pair.first);
        }

        for (int fd : fds_to_check) {
            if (FD_ISSET(fd, &read_fds)) {
                // Check if the FD is still valid (not removed by previous handler calls)
                if (client_fds.count(fd)) { // client_fds.count(fd) checks if key 'fd' exists
                    handle_client_message(fd);
                }
            }
        }
    }

    // 6. Graceful Shutdown
    cleanup_sockets();
    cout << "networkMonitor exiting." << endl;

    return 0;
}