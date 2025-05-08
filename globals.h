#ifndef GLOBALS_H
#define GLOBALS_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

// Constants for message handling
#define MAX_RECEIVERS 10
#define MAX_CONTENT 100
#define MAX_MESSAGES 100

// Keys for IPC resources
key_t sem_io_key;
key_t messages_key;
key_t num_readers_key;
key_t read_sem_key;
key_t sem_idx_key;
key_t in_idx_key;

// Define message structure with fixed-size arrays
struct message {
    int sender_id;
    int receivers[MAX_RECEIVERS];  // End marked by -1
    int num_recv;
    char content[MAX_CONTENT];
};

// Union for semctl operations
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

// Initialize the keys
void init_keys() {
    sem_io_key = ftok(".", 'a');
    messages_key = ftok(".", 'b');
    num_readers_key = ftok(".", 'c');
    read_sem_key = ftok(".", 'd');
    sem_idx_key = ftok(".", 'e');
    in_idx_key = ftok(".", 'f');
}

#endif // GLOBALS_H