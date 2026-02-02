// File: file_generator.c & the initials are dhsb.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

// Function to generate binary file filled with random data
void dhsb_generate_binary_file(const char *filename, int size_mb) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("File creation failed");
        exit(1);
    }

    char buffer[1024 * 1024]; // 1MB buffer
    memset(buffer, 'X', sizeof(buffer)); // Fill buffer with 'X'

    for (int i = 0; i < size_mb; i++) {
        ssize_t written = write(fd, buffer, sizeof(buffer));
        if (written < 0) {
            perror("Write failed");
            close(fd);
            exit(1);
        }
    }

    close(fd);
}

int main() {
    int size1, size2, size3;
    printf("Enter file size for Worker 1 (MB): ");
    scanf("%d", &size1);
    printf("Enter file size for Worker 2 (MB): ");
    scanf("%d", &size2);
    printf("Enter file size for Worker 3 (MB): ");
    scanf("%d", &size3);

    dhsb_generate_binary_file("worker1.bin", size1);
    dhsb_generate_binary_file("worker2.bin", size2);
    dhsb_generate_binary_file("worker3.bin", size3);

    printf("Binary files created.\n");
    return 0;
}
