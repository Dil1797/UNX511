// File: dhsb_logger.c

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Function to securely log messages to syslog.log using file locking
void dhsb_log_event(const char *message) {
    int fd = open("syslog.log", O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0) {
        perror("Log file open failed");
        return;
    }

    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;  // Lock the whole file
    lock.l_pid = getpid();

    // Apply blocking write lock
    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("File lock failed");
        close(fd);
        return;
    }

    // Write the message to the log file
    write(fd, message, strlen(message));

    // Release the lock
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);

    close(fd);
}

// Function to generate a binary file with dummy data of specified size in MB
void dhsb_generate_binary_file(const char *filename, int size_mb) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("File creation failed");
        exit(1);
    }

    char buffer[1024 * 1024]; // 1MB buffer
    memset(buffer, 'X', sizeof(buffer)); // Fill buffer with dummy data

    for (int i = 0; i < size_mb; i++) {
        if (write(fd, buffer, sizeof(buffer)) < 0) {
            perror("Write failed");
            close(fd);
            exit(1);
        }
    }

    close(fd);
}

