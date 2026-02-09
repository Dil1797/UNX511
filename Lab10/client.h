// client.h - Header file for shared memory and synchronization
//
// 23-Jul-20  M. Watler         Created.
// 06-Aug-25  AI Assistant      Updated with synchronization features.
//
#ifndef CLIENT_H
#define CLIENT_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>  // For POSIX semaphores
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

const int BUF_LEN=1024;
const int NUM_MESSAGES=30;

// Define the shared memory structure used by all clients
struct Memory {
    int            packet_no;
    unsigned short srcClientNo;
    unsigned short destClientNo;
    char           message[BUF_LEN];
};

// One semaphore is used as a mutex for synchronizing access to shared memory
#define MUTEX_SEM_NAME "/shm_mutex"
// The second semaphore is used to signal when client 3 has started
#define START_SEM_NAME "/shm_start"

extern sem_t *mutex_sem;
extern sem_t *start_sem;
extern struct Memory *ShmPTR;

void cleanup_clients();
void *recv_func(void *arg);

#endif //CLIENT_H
