// File: test_worker_main.c
#define _POSIX_C_SOURCE 200809L // Required for sigaction and other POSIX features

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // for fork(), getppid()
#include <sys/types.h>  // for pid_t
#include <sys/wait.h>   // for wait(), WIFEXITED, WIFSIGNALED, WTERMSIG, WEXITSTATUS
#include <signal.h>     // for sigaction, siginfo_t, SIGUSR1, SIGUSR2, sigemptyset
#include <string.h>     // for memset, strncmp, strsignal
#include <errno.h>      // for errno, perror
#include <fcntl.h>      // for open, O_WRONLY, O_CREAT, O_TRUNC

// Declare the worker process function from worker_process.c
// It's good practice to have this in a header file (e.g., worker_process.h)
// but for this example, we'll use extern.
extern void dhsb_worker_process(const char *filename, pid_t parent_pid);

// Global flags to track received signals (volatile for signal safety)
volatile sig_atomic_t sigusr1_received = 0;
volatile sig_atomic_t sigusr2_received = 0;

// Signal handler function for the parent process
// This function will be called when SIGUSR1 or SIGUSR2 is received.
// siginfo_t *info provides details about the signal, including the sender's PID.
void test_parent_signal_handler(int sig, siginfo_t *info, void *context) {
    // In a real application, be cautious with printf in signal handlers
    // as it's not strictly async-signal-safe. For testing, it's generally fine.
    if (sig == SIGUSR1) {
        printf("Parent (PID %d): Received SIGUSR1 (Memory warning) from Worker PID %d\n", getpid(), info->si_pid);
        sigusr1_received = 1; // Set flag to indicate SIGUSR1 was received
    } else if (sig == SIGUSR2) {
        printf("Parent (PID %d): Received SIGUSR2 (Completion) from Worker PID %d\n", getpid(), info->si_pid);
        sigusr2_received = 1; // Set flag to indicate SIGUSR2 was received
    }
}

// Function to set up signal handlers for SIGUSR1 and SIGUSR2 in the parent
void setup_parent_signal_handlers() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa)); // Initialize the sigaction struct to all zeros

    sa.sa_sigaction = test_parent_signal_handler; // Assign our custom handler
    sa.sa_flags = SA_SIGINFO; // Use sa_sigaction and get extended signal info
    sigemptyset(&sa.sa_mask); // Do not block any other signals while in the handler

    // Register the handler for SIGUSR1
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction SIGUSR1 failed in parent");
        exit(EXIT_FAILURE);
    }

    // Register the handler for SIGUSR2
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("sigaction SIGUSR2 failed in parent");
        exit(EXIT_FAILURE);
    }

    printf("Parent: Signal handlers set up for SIGUSR1 and SIGUSR2.\n");
}

// Helper function to create a dummy binary file for the worker to process
// This simulates the "Part 1: Dynamic File Generation" in this test executable.
void create_dummy_binary_file(const char *filename, int size_mb) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to create dummy file");
        exit(EXIT_FAILURE);
    }

    char buffer[1024 * 1024]; // 1MB buffer
    memset(buffer, 'X', sizeof(buffer)); // Fill buffer with dummy data (e.g., 'X' characters)

    printf("Parent: Creating dummy file '%s' of %d MB...\n", filename, size_mb);
    for (int i = 0; i < size_mb; i++) {
        // Write 1MB chunks to the file
        if (write(fd, buffer, sizeof(buffer)) != sizeof(buffer)) {
            perror("Error writing to dummy file");
            close(fd);
            exit(EXIT_FAILURE);
        }
    }
    close(fd);
    printf("Parent: Dummy file '%s' created.\n", filename);
}

int main() {
    // Define the name and size of the test binary file
    const char *test_filename = "worker_test_file.bin";
    // Create a sufficiently large file (e.g., 60MB) to ensure VmRSS exceeds 50MB
    create_dummy_binary_file(test_filename, 60);

    // Set up signal handlers in the parent process BEFORE forking any children.
    // This ensures the parent is ready to receive signals from its worker.
    setup_parent_signal_handlers();

    pid_t pid = fork(); // Create a child process

    if (pid < 0) {
        // Fork failed
        perror("fork failed");
        unlink(test_filename); // Clean up the dummy file
        return EXIT_FAILURE;
    } else if (pid == 0) {
        // Child process: This is where the worker logic runs
        printf("Worker: Child process (PID %d) starting for file '%s'. Parent PID: %d\n", getpid(), test_filename, getppid());
        // Call the worker process function, passing its parent's PID
        dhsb_worker_process(test_filename, getppid());
        // The dhsb_worker_process function is expected to call exit(0) itself.
        // If it returns, it indicates an unexpected flow, so exit with failure.
        fprintf(stderr, "Worker process returned unexpectedly.\n");
        exit(EXIT_FAILURE);
    } else {
        // Parent process: Waits for the child and verifies signals
        printf("Parent: Forked worker with PID %d. Waiting for worker completion and signals.\n", pid);

        int status;
        pid_t result_pid;
        // Loop to handle EINTR (Interrupted system call)
        // waitpid can be interrupted by signal handlers. We should retry if that happens.
        do {
            result_pid = waitpid(pid, &status, 0);
        } while (result_pid == -1 && errno == EINTR);

        if (result_pid == -1) {
            // A real error other than EINTR occurred
            perror("waitpid failed in parent after retry");
            unlink(test_filename); // Clean up the dummy file
            return EXIT_FAILURE;
        }

        // Check how the child terminated
        if (WIFEXITED(status)) {
            printf("Parent: Worker (PID %d) exited with status %d.\n", pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Parent: Worker (PID %d) terminated by signal %d (%s).\n", pid, WTERMSIG(status), strsignal(WTERMSIG(status)));
        }

        // Report on whether the expected signals were received
        if (sigusr1_received) {
            printf("Parent: Test result: SIGUSR1 (Memory warning) was successfully received.\n");
        } else {
            printf("Parent: Test result: WARNING: SIGUSR1 (Memory warning) was NOT received. "
                   "Ensure the dummy file is large enough or memory usage logic is correct.\n");
        }
        if (sigusr2_received) {
            printf("Parent: Test result: SIGUSR2 (Completion) was successfully received.\n");
        } else {
            printf("Parent: Test result: ERROR: SIGUSR2 (Completion) was NOT received. "
                   "Worker might not have completed or signaled correctly.\n");
            // If SIGUSR2 was not received, it's a critical failure for Part 2's objectives.
            unlink(test_filename);
            return EXIT_FAILURE;
        }

        printf("Parent: Test finished. Cleaning up dummy file.\n");
    }

    // Clean up the dummy binary file created for the test
    unlink(test_filename);

    return EXIT_SUCCESS; // Indicate successful test execution
}

