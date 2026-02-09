// client1.cpp - An exercise with named semaphores and shared memory
//
// 23-Jul-20  M. Watler         Created.
// 06-Aug-25  AI Assistant      Updated with synchronization features.
//
#include <errno.h>
#include <iostream>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "client.h"

using namespace std;
const int CLIENT_NO=1;   // This identifies this process as Client 1
bool is_running=true;

sem_t *mutex_sem;
sem_t *start_sem;
struct Memory *ShmPTR = NULL;

void cleanup_clients() {
    if (ShmPTR != NULL && ShmPTR != (void *)-1) {
        shmdt((void *)ShmPTR);
    }
    if (mutex_sem != SEM_FAILED) {
        sem_close(mutex_sem);
    }
    if (start_sem != SEM_FAILED) {
        sem_close(start_sem);
    }
}

static void sigHandler(int sig)
{
    switch(sig) {
        case SIGINT:
            is_running=false;
            break;
    }
}

int main(void) {
    key_t          ShmKey;
    int            ShmID;

    struct sigaction action;
    action.sa_handler = sigHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, NULL);

    atexit(cleanup_clients);

    // Add retry loop to wait for client3 to create the semaphores
    // Wait for client 3 to create the mutex semaphore before continuing
    int retries = 10;
    while (retries > 0) {
        mutex_sem = sem_open(MUTEX_SEM_NAME, 0);
        if (mutex_sem != SEM_FAILED) {
            break;
        }
        cout << "client1: Waiting for mutex semaphore..." << endl;
        sleep(1);
        retries--;
    }
    if (mutex_sem == SEM_FAILED) {
        cout << "client1: Failed to open mutex semaphore after multiple attempts" << endl;
        return -1;
    }
// Wait for start signal from client 3 (synchronization point)
    retries = 10;
    while (retries > 0) {
        start_sem = sem_open(START_SEM_NAME, 0);
        if (start_sem != SEM_FAILED) {
            break;
        }
        cout << "client1: Waiting for start semaphore..." << endl;
        sleep(1);
        retries--;
    }
    if (start_sem == SEM_FAILED) {
        cout << "client1: Failed to open start semaphore after multiple attempts" << endl;
        sem_close(mutex_sem);
        return -1;
    }

    cout << "Client 1 is waiting for client 3 to start..." << endl;
    sem_wait(start_sem);   // Block here until client 3 signals start
    cout << "Client 1 has been signaled by client 3. Starting communication..." << endl;

    // Connect to the shared memory segment
    ShmKey = 1234;
    ShmID = shmget(ShmKey, sizeof(struct Memory), 0);
    if (ShmID < 0) {
        cout << "client1: shmget() error" << endl;
        cout << strerror(errno) << endl;
        return -1;
    }

    ShmPTR = (struct Memory *) shmat(ShmID, NULL, 0);
    if (ShmPTR == (void *)-1) {
        cout << "client1: shmat() error" << endl;
        cout << strerror(errno) << endl;
        return -1;
    }

     // Main loop: Check for messages, process them, and respond
    for(int i=0; i<NUM_MESSAGES && is_running; ++i) {
        sem_wait(mutex_sem);   // Lock access to shared memory

        if(ShmPTR->destClientNo == CLIENT_NO) {
            cout << "Client " << CLIENT_NO << " has received a message from client " << ShmPTR->srcClientNo << ":" << endl;
            cout << ShmPTR->message << endl;

	    // Prepare a reply message to send to another client
            ShmPTR->srcClientNo = CLIENT_NO;
            ShmPTR->destClientNo = 2 + (i % 2);  // Alternate between client 2 and 3
            memset(ShmPTR->message, 0, BUF_LEN);
            sprintf(ShmPTR->message, "This is message %d from client %d", i+1, CLIENT_NO);
        }

        sem_post(mutex_sem);   // Unlock shared memory
        sleep(1);  // Delay to simulate communication interval
    }

    cout<<"client1: DONE"<<endl;

    return 0;
}
