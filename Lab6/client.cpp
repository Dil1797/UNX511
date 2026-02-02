#include <iostream>      // For standard input/output
#include <sys/socket.h>  // For socket functions
#include <sys/un.h>      // For Unix domain socket structures
#include <unistd.h>      // For close(), read(), write(), sleep(), getpid()
#include <cstring>       // For memset(), strcmp()

#define SOCKET_PATH "/tmp/lab6"  // Defines the socket file path *

int main() {
    int client_fd;  // File descriptor for client socket
    struct sockaddr_un server_addr;  // Address structure for server
    
    // Create a client socket (AF_UNIX: Unix domain socket, SOCK_STREAM: Reliable, connection-based)  *
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("Socket creation failed");  // Print error if socket creation fails
        return 1;
    }

    // Set up the server address structure
    memset(&server_addr, 0, sizeof(server_addr));  // Clear the memory for structure
    server_addr.sun_family = AF_UNIX;  // Set address family to AF_UNIX
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);  // Set socket file path

    // Connect to the server  *         *
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection to server failed");  // Print error if connection fails
        close(client_fd);
        return 1;
    }
    std::cout << "Connected to the server.\n";

    char buffer[256] = {0};  // Buffer to store server commands

    while (true) {  // Infinite loop to keep client running until told to quit
        // Read command from the server. The client keeps listening for commands from the server.  *
        memset(buffer, 0, sizeof(buffer));  // Clear buffer
        read(client_fd, buffer, sizeof(buffer));
        std::cout << "Client received command: " << buffer << "\n";

        // Respond to  "Pid" command  .  client responds with its own process ID.  *
        if (strcmp(buffer, "Pid") == 0) {
            std::string pid_response = "This client has pid " + std::to_string(getpid());
            write(client_fd, pid_response.c_str(), pid_response.length());
        }
        // Respond to "Sleep" command  . the client waits for 5 seconds and then sends "Done".  *
        else if (strcmp(buffer, "Sleep") == 0) {
            std::cout << "Client sleeping for 5 seconds...\n";
            sleep(5);  // Sleep for 5 seconds
            write(client_fd, "Done", 4);
        }
        // Respond to "Quit" command .  when  server sends "Quit", the client shuts down. *
        else if (strcmp(buffer, "Quit") == 0) {
            std::cout << "Client shutting down.\n";
            break;  // Exit the loop and shutdown
        }
    }

    // Close the socket and exit  *
    close(client_fd);
    return 0;
}


