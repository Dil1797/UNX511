#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>     // For sleep(), fork(), execve()
#include <signal.h>     // For signal()
#include <vector>       // For storing socket FDs (in networkMonitor - though not directly used here)
#include <sys/socket.h> // For socket(), connect(), send(), recv()
#include <sys/un.h>     // For sockaddr_un (Unix domain sockets)
#include <cstdio>       // For remove() (unlink)
#include <cstring>      // For memset, strncpy
#include <errno.h>      // For errno and perror

// For IOCTL (to set link up/down) - uncommented and implemented
#include <net/if.h>
#include <sys/ioctl.h>

using namespace std;

// Define a common socket path
#define SOCKET_PATH "/tmp/network_monitor_socket"
#define BUFFER_SIZE 256 // For socket communication messages

// Global flag for graceful shutdown
volatile sig_atomic_t running = 1;
int client_socket_fd = -1; // Global to allow signal handler to close

// Signal handler for Ctrl-C (SIGINT)
void sig_handler(int signo) {
    if (signo == SIGINT) {
        running = 0;
#ifdef DEBUG
        cerr << "DEBUG: intfMonitor received SIGINT. Initiating shutdown." << endl;
#endif
    }
}

// Function to read a single numerical value from a sysfs file
long read_sysfs_long(const string& path) {
    ifstream file(path);
    long value = 0;
    if (file.is_open()) {
        file >> value;
        file.close();
    }
    else {
        // This is a warning, typically kept on regardless of DEBUG flag
        cerr << "Warning: intfMonitor failed to open " << path << endl;
    }
    return value;
}

// Function to read the operstate (string value)
string read_sysfs_string(const string& path) {
    ifstream file(path);
    string value = "unknown"; // Default state if file can't be read
    if (file.is_open()) {
        file >> value;
        file.close();
    }
    else {
        // This is a warning, typically kept on regardless of DEBUG flag
        cerr << "Warning: intfMonitor failed to open " << path << endl;
    }
    return value;
}

// Function to send a message over the socket
void send_message(int sock_fd, const string& message) {
    if (sock_fd != -1) {
        if (send(sock_fd, message.c_str(), message.length(), 0) == -1) {
            perror("intfMonitor send");
        }
    }
}

// Function to receive a message over the socket
string receive_message(int sock_fd) {
    if (sock_fd == -1) return "";
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    ssize_t bytes_received = recv(sock_fd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0'; // Null-terminate
#ifdef DEBUG
        cerr << "DEBUG: intfMonitor received '" << buffer << "' from NM." << endl;
#endif
        return string(buffer);
    }
    else if (bytes_received == 0) {
        // Connection closed by peer
#ifdef DEBUG
        cerr << "DEBUG: Network Monitor closed connection." << endl;
#endif
        running = 0; // Trigger shutdown
    }
    else {
        // Error on recv, but check if it's due to SIGINT interrupting
        if (errno == EINTR) {
#ifdef DEBUG
            cerr << "DEBUG: recv interrupted by signal (likely SIGINT)." << endl;
#endif
        }
        else {
            perror("intfMonitor recv");
        }
    }
    return "";
}


int main(int argc, char* argv[]) {
    // 1. Check for command line arguments
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <interface-name>" << endl;
        return 1;
    }
    string interface_name = argv[1];

    // 2. Set up SIGINT handler for graceful shutdown
    signal(SIGINT, sig_handler);

    // 3. Create and connect a socket to the Network Monitor
    client_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_socket_fd == -1) {
        perror("intfMonitor socket");
        return 1;
    }

    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

#ifdef DEBUG
    cout << "DEBUG: intfMonitor for " << interface_name << " attempting to connect to " << SOCKET_PATH << endl;
#endif
    if (connect(client_socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("intfMonitor connect");
        close(client_socket_fd);
        return 1;
    }
#ifdef DEBUG
    cout << "DEBUG: intfMonitor for " << interface_name << " connected." << endl;
#endif

    // 4. Implement the communication protocol
    // Send "Ready" to Network Monitor
    send_message(client_socket_fd, "Ready");
#ifdef DEBUG
    cout << "DEBUG: intfMonitor for " << interface_name << " sent 'Ready'." << endl;
#endif

    // Wait for "Monitor" from Network Monitor
    string nm_response = receive_message(client_socket_fd);
    if (nm_response != "Monitor") {
        cerr << "ERROR: intfMonitor expected 'Monitor', got: " << nm_response << endl;
        running = 0; // Unexpected message, terminate
    }
    else {
#ifdef DEBUG
        cout << "DEBUG: intfMonitor for " << interface_name << " received 'Monitor'." << endl;
#endif
        // Respond with "Monitoring"
        send_message(client_socket_fd, "Monitoring");
#ifdef DEBUG
        cout << "DEBUG: intfMonitor for " << interface_name << " sent 'Monitoring'." << endl;
#endif
    }

    // 5. Main monitoring loop
    while (running) {
        string base_path = "/sys/class/net/" + interface_name + "/";
        string stats_path = base_path + "statistics/";

        // Read all required statistics from sysfs
        string operstate = read_sysfs_string(base_path + "operstate");
        long up_count = read_sysfs_long(base_path + "carrier_up_count");
        long down_count = read_sysfs_long(base_path + "carrier_down_count");

        long rx_bytes = read_sysfs_long(stats_path + "rx_bytes");
        long rx_dropped = read_sysfs_long(stats_path + "rx_dropped");
        long rx_errors = read_sysfs_long(stats_path + "rx_errors");
        long rx_packets = read_sysfs_long(stats_path + "rx_packets");

        long tx_bytes = read_sysfs_long(stats_path + "tx_bytes");
        long tx_dropped = read_sysfs_long(stats_path + "tx_dropped");
        long tx_errors = read_sysfs_long(stats_path + "tx_errors");
        long tx_packets = read_sysfs_long(stats_path + "tx_packets");

        // --- Link Down Logic ---
        if (operstate == "down") {
#ifdef DEBUG
            cout << "DEBUG: " << interface_name << " is down. Reporting to Network Monitor." << endl;
#endif
            send_message(client_socket_fd, "Link Down");

            // Wait for "Set Link Up" command from Network Monitor (blocking for simplicity here)
            // In a real scenario, you might want a select() call with a timeout here
            // to allow for other processing.
            string link_cmd = receive_message(client_socket_fd);
            if (link_cmd == "Set Link Up") {
#ifdef DEBUG
                cout << "DEBUG: intfMonitor for " << interface_name << " received 'Set Link Up'. Attempting to bring link up." << endl;
#endif
                // --- IOCTL to set link UP ---
                // This typically requires root privileges.
                struct ifreq ifr;
                int sock = socket(AF_INET, SOCK_DGRAM, 0); // DGRAM socket for ioctl
                if (sock == -1) {
                    perror("socket for ioctl");
                }
                else {
                    strncpy(ifr.ifr_name, interface_name.c_str(), IFNAMSIZ - 1);
                    ifr.ifr_flags |= IFF_UP; // Set the UP flag

                    if (ioctl(sock, SIOCSIFFLAGS, &ifr) == -1) {
                        perror("ioctl SIOCSIFFLAGS (setting link up)");
                    }
                    else {
#ifdef DEBUG
                        cout << "DEBUG: " << interface_name << " link should now be up." << endl;
#endif
                    }
                    close(sock);
                }
                // --- End IOCTL ---
            }
            else if (link_cmd == "Shut Down") {
                running = 0; // NM sent shutdown during link down process
            }
        }

        // Print the statistics to stdout in the EXACT required format
        cout << "Interface:" << interface_name << " state:" << operstate
            << " up_count:" << up_count << " down_count:" << down_count << endl;
        cout << "rx_bytes:" << rx_bytes << " rx_dropped:" << rx_dropped
            << " rx_errors:" << rx_errors << " rx_packets:" << rx_packets << endl;
        cout << "tx_bytes:" << tx_bytes << " tx_dropped:" << tx_dropped
            << " tx_errors:" << tx_errors << " tx_packets:" << tx_packets << endl;
        cout << endl; // Add an empty line for readability between updates

        // Check for "Shut Down" message from networkMonitor (non-blocking or with timeout)
        // This is a simplified approach, in a real select() loop you'd handle this.
        // For synchronous communication as specified:
        // After sending Monitoring, you expect only "Set Link Up" or "Shut Down"
        // so you might not need a separate recv here if you're waiting for
        // "Set Link Up" or if NM only sends "Shut Down" on exit.
        // If the socket is non-blocking, you could try a quick read.
        // For simplicity, let's assume the main control is via the Link Up/Down flow
        // or the initial connection response. The SIGINT handler will catch external signals.

        sleep(1); // Poll every second
    }

    // 6. Graceful Shutdown
#ifdef DEBUG
    cout << "DEBUG: intfMonitor for " << interface_name << " is shutting down." << endl;
#endif
    // Inform Network Monitor that we are done (if socket is still open)
    if (client_socket_fd != -1) {
        send_message(client_socket_fd, "Done");
        close(client_socket_fd);
        client_socket_fd = -1;
    }
#ifdef DEBUG
    cout << "DEBUG: intfMonitor for " << interface_name << " exited gracefully." << endl;
#endif

    return 0;
}



