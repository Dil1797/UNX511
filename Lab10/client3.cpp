// client3.cpp - An exercise with named semaphores and shared memory
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
const int CLIENT_NO=3;  // Client number for identification
bool is_running=true;   // Flag for shutdown

sem_t *mutex_sem;
sem_t *start_sem;
struct Memory *ShmPTR = NULL;

// Clean up shared memory and semaphores on exit
void cleanup_clients() {
    if (ShmPTR != NULL && ShmPTR != (void *)-1) {
        shmdt((void *)ShmPTR);  // Detach shared memory
        shmctl(shmget(1234, 0, 0), IPC_RMID, NULL);   // Delete shared memory
    }
    if (mutex_sem != SEM_FAILED) {
        sem_close(mutex_sem);     // Close mutex semaphore
        sem_unlink(MUTEX_SEM_NAME);    // Unlink semaphore
    }
    if (start_sem != SEM_FAILED) {
        sem_close(start_sem);       // Close start semaphore
        sem_unlink(START_SEM_NAME);   // Unlink semaphore
    }
}

// Handle Ctrl+C signal to stop the loop cleanly
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

    // Setup signal handling
    struct sigaction action;
    action.sa_handler = sigHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, NULL);

    atexit(cleanup_clients);   // Register cleanup on exit


      // Create mutex semaphore with initial value 1 (binary semaphore)
    mutex_sem = sem_open(MUTEX_SEM_NAME, O_CREAT, 0666, 1);
    if (mutex_sem == SEM_FAILED) {
        cout << "client3: sem_open() error for mutex" << endl;
        cout << strerror(errno) << endl;
        return -1;
    }

      // Create start semaphore initialized to 0
    start_sem = sem_open(START_SEM_NAME, O_CREAT, 0666, 0);
    if (start_sem == SEM_FAILED) {
        cout << "client3: sem_open() error for start sem" << endl;
        cout << strerror(errno) << endl;
        return -1;
    }

       // Create shared memory segment
    ShmKey = 1234;
    ShmID = shmget(ShmKey, sizeof(struct Memory), IPC_CREAT | 0666);
    if (ShmID < 0) {
        cout << "client3: shmget() error" << endl;
        cout << strerror(errno) << endl;
        return -1;
    }

      // Attach to the shared memory
    ShmPTR = (struct Memory *) shmat(ShmID, NULL, 0);
    if (ShmPTR == (void *)-1) {
        cout << "client3: shmat() error" << endl;
        cout << strerror(errno) << endl;
        return -1;
    }

      // Initialize the first message in shared memory
    ShmPTR->srcClientNo=CLIENT_NO;
    ShmPTR->destClientNo=1;
    memset(ShmPTR->message, 0, BUF_LEN);
    sprintf(ShmPTR->message, "This is message 0 from client %d", CLIENT_NO);
    cout << "Client 3 has initialized shared memory and is starting..." << endl;

      // Signal client1 and client2 to start
    sem_post(start_sem);
    sem_post(start_sem);

        // Main loop to handle message exchange
    for(int i=0; i<NUM_MESSAGES && is_running; ++i) {
        sem_wait(mutex_sem);  // Lock access to shared memory

	   // If message is meant for client 3
        if(ShmPTR->destClientNo == CLIENT_NO) {
            cout << "Client " << CLIENT_NO << " has received a message from client " << ShmPTR->srcClientNo << ":" << endl;
            cout << ShmPTR->message << endl;

	   // Prepare response message
            ShmPTR->srcClientNo = CLIENT_NO;
            ShmPTR->destClientNo = 1 + (i % 2);  // Alternate between client1 and client2
            memset(ShmPTR->message, 0, BUF_LEN);
            sprintf(ShmPTR->message, "This is message %d from client %d", i+1, CLIENT_NO);
        }

        sem_post(mutex_sem);   // Release shared memory lock
        sleep(1);     // Delay for readability
   }

    cout << "client3: DONE"<<endl;

    return 0;
}
