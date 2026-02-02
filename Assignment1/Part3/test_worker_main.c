#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// Track number of signals received
volatile sig_atomic_t dhsb_signals_received = 0;

// Signal handler
void dhsb_signal_handler(int sig, siginfo_t *si, void *context) {
    if (sig == SIGUSR1) {
        printf("[Parent] Received SIGUSR1: Worker PID %d exceeded memory limit!\n", si->si_pid);
    } else if (sig == SIGUSR2) {
        printf("[Parent] Received SIGUSR2: Worker PID %d completed its task.\n", si->si_pid);
    }
    dhsb_signals_received++;
}

// Setup signal handlers
void dhsb_setup_signals() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = dhsb_signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction SIGUSR1");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("sigaction SIGUSR2");
        exit(EXIT_FAILURE);
    }
}

// Forward declaration (worker_process.c)
void dhsb_worker_process(const char *filename, pid_t parent_pid);

int main() {
    pid_t pid;
    const char *filename = "worker1.bin";

    dhsb_setup_signals(); // Install signal handlers

    pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Child: run the worker
        dhsb_worker_process(filename, getppid());
        exit(EXIT_SUCCESS);
    } else {
        // Parent: wait for 2 signals from worker
        while (dhsb_signals_received < 2) {
            pause(); // suspend until a signal is caught
        }

        waitpid(pid, NULL, 0);
        printf("[Parent] Done waiting.\n");
    }

    return 0;
}


