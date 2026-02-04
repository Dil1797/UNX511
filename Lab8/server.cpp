// server.cpp - Server for message queue lab

#include <iostream>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <cstring>
#include <csignal>
#include <unistd.h>
#include <cstdio>   // for perror()
#include "client.h"

using namespace std;

int msgid;
bool is_running = true;

static void shutdownHandler(int sig) {
    if (sig == SIGINT) {
        is_running = false;
        cout << "\n[Server] Shutdown requested..." << endl;
    }
}

int main() {
    key_t key = ftok("serverclient", 65);
    msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("[Server] msgget failed");
        return 1;
    }

    // Setup signal handler for Ctrl+C
    struct sigaction action;
    action.sa_handler = shutdownHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, NULL);

    Message msg;

    cout << "[Server] Started. Press Ctrl+C to stop." << endl;

    // Main server loop: receive and forward messages
    while (is_running) {
        ssize_t ret = msgrcv(msgid, &msg, sizeof(msg), 4, IPC_NOWAIT);
        if (ret == -1) {
            // No message to receive, sleep shortly
            usleep(100000);
            continue;
        }

        cout << "[Server] Received from client " << msg.msgBuf.source
             << " for client " << msg.msgBuf.dest << ": " << msg.msgBuf.buf;

        // Forward the message to destination client
        msg.mtype = msg.msgBuf.dest;
        if (msgsnd(msgid, &msg, sizeof(msg), 0) == -1) {
            perror("[Server] msgsnd failed");
        } else {
            cout << "[Server] Forwarded to client " << msg.msgBuf.dest << endl;
        }
    }

    // On shutdown: send Quit message to all clients
    for (int i = 1; i <= 3; i++) {
        msg.mtype = i;
        strcpy(msg.msgBuf.buf, "Quit");
        msgsnd(msgid, &msg, sizeof(msg), 0);
        cout << "[Server] Sent Quit to client " << i << endl;
    }

    // Remove message queue
    msgctl(msgid, IPC_RMID, NULL);
    cout << "[Server] Shutdown complete. Goodbye!" << endl;

    return 0;
}


