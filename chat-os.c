#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <string.h>
#include "globals.h"

// Helper function to initialize a semaphore
int init_semaphore(key_t key, int initial_value) {
    int semid = semget(key, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("Failed to create semaphore");
        exit(EXIT_FAILURE);
    }
    
    union semun arg;
    arg.val = initial_value;
    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("Failed to initialize semaphore");
        exit(EXIT_FAILURE);
    }
    
    return semid;
}

// Helper function to clean up IPC resources
void cleanup_resources(int sem_io_id, int sem_read_id, int sem_idx_id,
                       int messages_id, int num_readers_id, int in_idx_id) {
    // Clean up semaphores
    if (semctl(sem_io_id, 0, IPC_RMID) == -1) {
        perror("Failed to remove sem_io");
    }
    
    if (semctl(sem_read_id, 0, IPC_RMID) == -1) {
        perror("Failed to remove sem_read");
    }
    
    if (semctl(sem_idx_id, 0, IPC_RMID) == -1) {
        perror("Failed to remove sem_idx");
    }
    
    // Clean up shared memory
    if (shmctl(messages_id, IPC_RMID, NULL) == -1) {
        perror("Failed to remove messages shared memory");
    }
    
    if (shmctl(num_readers_id, IPC_RMID, NULL) == -1) {
        perror("Failed to remove num_readers shared memory");
    }
    
    if (shmctl(in_idx_id, IPC_RMID, NULL) == -1) {
        perror("Failed to remove in_idx shared memory");
    }
}

int main() {
    // Initialize keys for IPC resources
    init_keys();
    
    // Create and initialize semaphores
    int sem_io_id = init_semaphore(sem_io_key, 1);  // Binary semaphore for I/O
    int sem_read_id = init_semaphore(read_sem_key, 1);  // Binary semaphore for readers
    int sem_idx_id = init_semaphore(sem_idx_key, 1);  // Binary semaphore for in_idx access
    
    // Create shared memory for messages
    int messages_id = shmget(messages_key, MAX_MESSAGES * sizeof(struct message), IPC_CREAT | 0666);
    if (messages_id == -1) {
        perror("Failed to create messages shared memory");
        exit(EXIT_FAILURE);
    }
    struct message *messages = (struct message *)shmat(messages_id, NULL, 0);
    if (messages == (void *)-1) {
        perror("Failed to attach to messages shared memory");
        exit(EXIT_FAILURE);
    }
    
    // Create shared memory for num_readers
    int num_readers_id = shmget(num_readers_key, sizeof(int), IPC_CREAT | 0666);
    if (num_readers_id == -1) {
        perror("Failed to create num_readers shared memory");
        exit(EXIT_FAILURE);
    }
    int *num_readers = (int *)shmat(num_readers_id, NULL, 0);
    if (num_readers == (void *)-1) {
        perror("Failed to attach to num_readers shared memory");
        exit(EXIT_FAILURE);
    }
    *num_readers = 0;  // Initialize to 0
    
    // Create shared memory for in_idx
    int in_idx_id = shmget(in_idx_key, sizeof(int), IPC_CREAT | 0666);
    if (in_idx_id == -1) {
        perror("Failed to create in_idx shared memory");
        exit(EXIT_FAILURE);
    }
    int *in_idx = (int *)shmat(in_idx_id, NULL, 0);
    if (in_idx == (void *)-1) {
        perror("Failed to attach to in_idx shared memory");
        exit(EXIT_FAILURE);
    }
    *in_idx = -1;  // Initialize to -1, indicating no messages
    
    // Prompt for number of chat clients
    int num_clients;
    printf("Enter the number of chat clients to create: ");
    scanf("%d", &num_clients);
    
    if (num_clients <= 0 || num_clients > MAX_RECEIVERS) {
        printf("Invalid number of clients. Must be between 1 and %d.\n", MAX_RECEIVERS);
        cleanup_resources(sem_io_id, sem_read_id, sem_idx_id, messages_id, num_readers_id, in_idx_id);
        exit(EXIT_FAILURE);
    }
    
    // Array to store child PIDs for later cleanup
    pid_t child_pids[num_clients];
    
    // Fork child processes
    for (int i = 0; i < num_clients; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("Fork failed");
            cleanup_resources(sem_io_id, sem_read_id, sem_idx_id, messages_id, num_readers_id, in_idx_id);
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process
            // Convert client ID to string for passing to child
            char client_id_str[10];
            sprintf(client_id_str, "%d", i);
            
            // Launch xterm with child process
            execlp("xterm", "xterm", "-title", client_id_str, "-e", "./child", client_id_str, NULL);
            
            // If execlp fails
            perror("Failed to execute child process");
            exit(EXIT_FAILURE);
        } else {
            // Parent process
            child_pids[i] = pid;
            printf("Started chat client %d with PID %d\n", i, pid);
        }
    }
    
    // Wait for all child processes to complete
    printf("Waiting for all chat clients to exit...\n");
    int status;
    for (int i = 0; i < num_clients; i++) {
        waitpid(child_pids[i], &status, 0);
        printf("Chat client %d exited with status %d\n", i, WEXITSTATUS(status));
    }
    
    // Clean up IPC resources
    printf("Cleaning up resources...\n");
    cleanup_resources(sem_io_id, sem_read_id, sem_idx_id, messages_id, num_readers_id, in_idx_id);
    
    printf("Chat system terminated.\n");
    return 0;
}