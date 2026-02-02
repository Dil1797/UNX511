#include <iostream>      // For standard input/output
#include <sys/socket.h>  // For socket functions
#include <sys/un.h>      // For Unix domain socket structures
#include <unistd.h>      // For close(), read(), write(), unlink()
#include <cstring>       // For memset(), strncpy()

#define SOCKET_PATH "/tmp/lab6"  // Defines the socket file path.  the file location where the server and client will communicate. *

int main() {
    int server_fd, client_fd;  // File descriptors for server and client sockets
    struct sockaddr_un server_addr, client_addr;  // Address structures for server and client
    socklen_t client_len = sizeof(client_addr);  // Size of client address structure

    // Create a socket (AF_UNIX: Unix domain socket, SOCK_STREAM: Reliable, connection-based)  *
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");  // Print error if socket creation fails
        return 1;
    }

    // Set up the server address structure
    memset(&server_addr, 0, sizeof(server_addr));  // Clear the memory for structure
    server_addr.sun_family = AF_UNIX;  // Set address family to AF_UNIX
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);  // Set socket file path

    //  Remove any existing socket file (if it exists) to avoid binding errors  *
    unlink(SOCKET_PATH);

    // Bind the socket to the specified path  *
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");  // Print error if binding fails
        close(server_fd);
        return 1;
    }

    // Listen for client connections (max 1 client can connect at a time)  *
    if (listen(server_fd, 1) == -1) {
        perror("Listen failed");  // Print error if listening fails
        close(server_fd);
        return 1;
    }
    std::cout << "Waiting for the client...\n";

    // When a client connects, the server accepts it and creates a new connection. and both can talk.  *
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) {
        perror("Accept failed");  // Print error if accept fails
        close(server_fd);
        return 1;
    }
    std::cout << "Client connected to the server\n";

    // The server sends a command "Pid" asking for the client’s process ID  *
    write(client_fd, "Pid", 3);
    
    // Receive response from client , read and print the response  * 
    char buffer[256] = {0};  // Buffer to store client response
    read(client_fd, buffer, sizeof(buffer));
    std::cout << "Server received: " << buffer << "\n";

    // The server tells the client to sleep for 5 seconds.  *
    write(client_fd, "Sleep", 5);
    
    //  Receive "Done" response from client after sleeping or client wakes up and responds with "Done"  *
    memset(buffer, 0, sizeof(buffer));  // Clear buffer
    read(client_fd, buffer, sizeof(buffer));
    std::cout << "Server received: " << buffer << "\n";

    //  Send "Quit" command to client  or  shut down .  *
    write(client_fd, "Quit", 4);

    // Close connections and cleanup .    *
    close(client_fd);   // Close client connection
    close(server_fd);   // Close server socket
    unlink(SOCKET_PATH); // Remove the socket file
    std::cout << "Server shutdown.\n";

    return 0;  // Exit successfully
}


