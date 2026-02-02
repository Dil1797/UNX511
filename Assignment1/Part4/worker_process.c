// File: worker_process.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>

// No need to include dhsb_logger.h here, as worker only sends signals, doesn't log directly.
// But it uses dhsb_generate_binary_file from the parent setup.

void dhsb_worker_process(const char *filename, pid_t parent_pid, int worker_id) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("Worker: Failed to open file");
        exit(1);
    }

    char buffer[4096];
    ssize_t bytes_read;
    int mem_usage = 0;
    int warned = 0;

    // Allocate memory based on worker ID (worker 3 gets more to exceed 50MB)
    int alloc_size_mb = 0;
    if (worker_id == 1) alloc_size_mb = 5; // Minimal allocation
    else if (worker_id == 2) alloc_size_mb = 40; // Moderate, might increase VmRSS but likely under 50MB
    else if (worker_id == 3) alloc_size_mb = 120; // Definitely exceeds 50MB for SIGUSR1

    char *mem_accum = malloc(alloc_size_mb * 1024 * 1024);
    if (!mem_accum) {
        perror("Worker: Memory allocation failed");
        close(fd);
        exit(1);
    }
    // Touch the memory to ensure it's truly resident (prevents lazy allocation issues)
    memset(mem_accum, 0, alloc_size_mb * 1024 * 1024);

    // Read the file in chunks
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        // In a real scenario, you'd process the data here.
        // For this simulation, we just read to simulate work.

        // Periodically check memory usage
        FILE *status_file = fopen("/proc/self/status", "r");
        if (!status_file) {
            // Error opening status file, but don't exit immediately, just skip check for this iteration
            perror("Worker: Failed to open /proc/self/status");
        } else {
            char line[256];
            while (fgets(line, sizeof(line), status_file)) {
                if (strncmp(line, "VmRSS:", 6) == 0) {
                    sscanf(line + 6, "%d", &mem_usage);  // VmRSS is in kB
                    break;
                }
            }
            fclose(status_file);

            // Check if memory limit exceeded and warn parent (only once)
            // 50000 KB = 50 MB
            if (!warned && mem_usage > 50000) {
                kill(parent_pid, SIGUSR1);   // Warn parent
                warned = 1;
                // Crucial delay: give parent time to process SIGUSR1 and print its message
                usleep(150000); // 150ms delay, increased slightly for robustness
            }
        }
        // Minimal delay to allow other processes to run and signals to be delivered
        usleep(100); // Small pause after each read operation
    }

    close(fd);
    // Notify parent of completion
    kill(parent_pid, SIGUSR2);
    free(mem_accum); // Free allocated memory
    exit(0); // Worker process exits
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <filename> <parent_pid> <worker_id>\n", argv[0]);
        exit(1);
    }

    const char *filename = argv[1];
    pid_t parent_pid = atoi(argv[2]);
    int worker_id = atoi(argv[3]);

    dhsb_worker_process(filename, parent_pid, worker_id);
    return 0;
}
