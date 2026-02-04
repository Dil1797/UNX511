#include <iostream>    // For input/output
#include <unistd.h>    // For fork(), pipe(), execvp()
#include <sys/types.h> // For pid_t
#include <sys/wait.h>  // For waitpid()
#include <cstring>     // For strcpy(), strtok()
#include <cstdio>      // For perror()

const int LEN = 64;       // Increased to avoid overflow
const int MAX_ARGS = 3;   // Maximum number of arguments per command

int main(int argc, char* argv[]) {
    if (argc != 3) { // Ensure two command arguments are provided
        std::cerr << "Usage: " << argv[0] << " \"command1\" \"command2\"" << std::endl;
        return 1;
    }

    // Copy arguments into local buffers
    char argument1[LEN];
    char argument2[LEN];
    strcpy(argument1, argv[1]);
    strcpy(argument2, argv[2]);

    // Arrays to store split command arguments
    char arg1[MAX_ARGS][LEN];
    char arg2[MAX_ARGS][LEN];
    int len1 = 0, len2 = 0;

    // Tokenize first command
    char* token = strtok(argument1, " ");
    while (token != NULL && len1 < MAX_ARGS) {
        strcpy(arg1[len1], token);
        token = strtok(NULL, " ");
        ++len1;
    }

    // Tokenize second command
    token = strtok(argument2, " ");
    while (token != NULL && len2 < MAX_ARGS) {
        strcpy(arg2[len2], token);
        token = strtok(NULL, " ");
        ++len2;
    }

    // Convert to char* arrays for execvp
    char* cmd1[MAX_ARGS + 1];
    char* cmd2[MAX_ARGS + 1];

    for (int i = 0; i < len1; ++i) {
        cmd1[i] = arg1[i];
    }
    cmd1[len1] = NULL;

    for (int i = 0; i < len2; ++i) {
        cmd2[i] = arg2[i];
    }
    cmd2[len2] = NULL;

    // Create pipe
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // Child process - run first command
        close(pipefd[0]);                      // Close read end
        dup2(pipefd[1], STDOUT_FILENO);        // Redirect stdout to pipe
        close(pipefd[1]);                      // Close write end after dup

        execvp(cmd1[0], cmd1);                 // Execute command
        perror("execvp");                      // If failed
        return 1;
    } else {
        // Parent process - run second command
        close(pipefd[1]);                      // Close write end
        dup2(pipefd[0], STDIN_FILENO);         // Redirect stdin to pipe
        close(pipefd[0]);                      // Close read end after dup

        execvp(cmd2[0], cmd2);                 // Execute command
        perror("execvp");                      // If failed
        return 1;
    }
}


