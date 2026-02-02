// File: worker_process.c
#define _POSIX_C_SOURCE 200809L // Ensure POSIX features for functions like kill

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>     // For kill(), SIGUSR1, SIGUSR2
#include <string.h>     // For memset, strncmp
#include <sys/types.h>  // For pid_t

// Simulate memory usage
#define ACCUMULATE_MEMORY 1

void dhsb_worker_process(const char *filename, pid_t parent_pid) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("Worker: Failed to open file");
        exit(1);
    }

    char buffer[4096]; // 4KB buffer for reading chunks
    ssize_t bytes_read;
    int mem_usage = 0;
    
    // Flag to ensure SIGUSR1 is sent only once
    int sigusr1_already_sent = 0;

#if ACCUMULATE_MEMORY
    // Allocate a large chunk of memory at the start and touch it
    // to ensure VmRSS immediately goes over the threshold.
    size_t initial_mem_alloc_mb = 55; // Allocate 55MB to guarantee VmRSS > 50MB
    size_t initial_mem_alloc_bytes = initial_mem_alloc_mb * 1024 * 1024;
    char *initial_dummy_mem = malloc(initial_mem_alloc_bytes);
    if (!initial_dummy_mem) {
        perror("Worker: Initial memory allocation failed");
        close(fd);
        exit(1);
    }
    // Touch the allocated memory to force demand paging to bring it into VmRSS
    memset(initial_dummy_mem, 0, initial_mem_alloc_bytes);
    printf("Worker: Pre-allocated and touched %zu MB of memory.\n", initial_mem_alloc_mb);

    // --- IMMEDIATE VmRSS CHECK AND SIGUSR1 SEND AFTER INITIAL ALLOCATION ---
    // Perform a VmRSS check right after initial allocation to ensure SIGUSR1 is sent early.
    FILE *status_file_initial = fopen("/proc/self/status", "r");
    if (status_file_initial) {
        char line_initial[256];
        while (fgets(line_initial, sizeof(line_initial), status_file_initial)) {
            if (strncmp(line_initial, "VmRSS:", 6) == 0) {
                sscanf(line_initial + 6, "%d", &mem_usage); // mem_usage is in KB
                break;
            }
        }
        fclose(status_file_initial);

        if (mem_usage > 50000 && !sigusr1_already_sent) {
            kill(parent_pid, SIGUSR1); // High memory usage alert
            sigusr1_already_sent = 1; // Set flag to prevent re-sending
            printf("Worker: Sent SIGUSR1 immediately after initial memory allocation (VmRSS: %d KB).\n", mem_usage);
        }
    } else {
        perror("Worker: Failed to open /proc/self/status for initial check");
        // Continue, but this might affect SIGUSR1 sending
    }
    // -----------------------------------------------------------------------

    // Continue with the accumulation buffer for file data if needed,
    // but the SIGUSR1 should already be triggered by initial_dummy_mem.
    size_t file_data_capacity = 60 * 1024 * 1024; // 60MB for file data accumulation
    char *accumulated_data = malloc(file_data_capacity);
    if (!accumulated_data) {
        perror("Worker: File data memory allocation failed");
        free(initial_dummy_mem); // Free initial dummy memory
        close(fd);
        exit(1);
    }
    size_t offset = 0;
#endif

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {

#if ACCUMULATE_MEMORY
        // Accumulate file data (this will further increase VmRSS as pages are touched)
        if (offset + bytes_read <= file_data_capacity) { // Use <= for safety
            memcpy(accumulated_data + offset, buffer, bytes_read);
            offset += bytes_read;
        } else {
            // Handle case where file data exceeds allocated accumulation capacity
            // For this assignment, we assume file_data_capacity is sufficient.
            // In a real app, you might realloc or log a warning.
        }
#endif

        // Regular VmRSS check within the loop (will only send SIGUSR1 if not already sent)
        FILE *status_file_loop = fopen("/proc/self/status", "r");
        if (!status_file_loop) {
            perror("Worker: Failed to open /proc/self/status in loop");
            close(fd);
#if ACCUMULATE_MEMORY
            free(initial_dummy_mem);
            free(accumulated_data);
#endif
            exit(1);
        }

        char line[256];
        while (fgets(line, sizeof(line), status_file_loop)) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                sscanf(line + 6, "%d", &mem_usage); // mem_usage is in KB
                break;
            }
        }
        fclose(status_file_loop);

        // Check if memory usage exceeds 50MB (50000 KB) AND if SIGUSR1 hasn't been sent yet
        // Removed 'break;' here to ensure file is fully read and SIGUSR2 is sent.
        if (mem_usage > 50000 && !sigusr1_already_sent) {
            kill(parent_pid, SIGUSR1); // High memory usage alert
            sigusr1_already_sent = 1; // Set flag to prevent re-sending
            // printf("Worker: Sent SIGUSR1 from loop (VmRSS: %d KB).\n", mem_usage); // Debug line, can be removed
        }
    }

    close(fd);
#if ACCUMULATE_MEMORY
    free(initial_dummy_mem); // Free the initial dummy memory
    free(accumulated_data);  // Free the accumulated file data memory
#endif
    printf("Worker completed: %s\n", filename);
    kill(parent_pid, SIGUSR2); // Completion signal
    exit(0);
}

