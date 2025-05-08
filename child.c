#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include "globals.h"

// Global variables for IPC resources
int sem_io_id, sem_read_id, sem_idx_id;
struct message *messages;
int *num_readers;
int *in_idx;
int client_id;
int out_idx = 0;  // Local variable to track which messages have been read

// Semaphore operations
void wait_sem(int semid) {
    struct sembuf sb = {0, -1, 0};
    if (semop(semid, &sb, 1) == -1) {
        perror("semop: wait operation failed");
        exit(EXIT_FAILURE);
    }
}

void signal_sem(int semid) {
    struct sembuf sb = {0, 1, 0};
    if (semop(semid, &sb, 1) == -1) {
        perror("semop: signal operation failed");
        exit(EXIT_FAILURE);
    }
}

// Reader function to read messages from shared memory
void reader() {
    // Check if there are new messages to read
    wait_sem(sem_idx_id);
    if (*in_idx <= out_idx - 1) {
        // No new messages
        signal_sem(sem_idx_id);
        return;
    }
    signal_sem(sem_idx_id);
    
    // Get access to read
    wait_sem(sem_read_id);
    if (*num_readers == 0) {
        wait_sem(sem_io_id);  // First reader blocks writers
    }
    (*num_readers)++;
    signal_sem(sem_read_id);
    
    // Read messages from shared memory
    printf("\n--- New Messages ---\n");
    int new_messages = 0;
    
    for (int i = out_idx; i <= *in_idx; i++) {
        // Check if this message is for this client
        int is_recipient = 0;
        for (int j = 0; j < messages[i].num_recv; j++) {
            if (messages[i].receivers[j] == client_id) {
                is_recipient = 1;
                break;
            }
            if (messages[i].receivers[j] == -1) break;
        }
        
        if (is_recipient) {
            printf("From Client %d: %s\n", messages[i].sender_id, messages[i].content);
            new_messages++;
        }
    }
    
    if (new_messages == 0) {
        printf("No new messages for you.\n");
    }
    printf("-------------------\n\n");
    
    // Update out_idx to the last read message
    out_idx = *in_idx + 1;
    
    // Release read access
    wait_sem(sem_read_id);
    (*num_readers)--;
    if (*num_readers == 0) {
        signal_sem(sem_io_id);  // Last reader unblocks writers
    }
    signal_sem(sem_read_id);
}

// Writer function to send a message
void send_message() {
    char input_buffer[MAX_CONTENT];
    int num_recipients;
    
    printf("Enter the number of recipients: ");
    if (scanf("%d", &num_recipients) != 1 || num_recipients <= 0 || num_recipients > MAX_RECEIVERS) {
        printf("Invalid number of recipients. Must be between 1 and %d.\n", MAX_RECEIVERS);
        // Clear input buffer
        while (getchar() != '\n');
        return;
    }
    
    // Buffer to store recipients
    int recipients[MAX_RECEIVERS];
    memset(recipients, -1, sizeof(recipients));  // Initialize with -1 to mark end
    
    // Get recipient IDs
    for (int i = 0; i < num_recipients; i++) {
        printf("Enter recipient %d ID: ", i + 1);
        if (scanf("%d", &recipients[i]) != 1) {
            printf("Invalid recipient ID.\n");
            // Clear input buffer
            while (getchar() != '\n');
            return;
        }
    }
    
    // Clear the newline from the input buffer
    while (getchar() != '\n');
    
    printf("Enter message content (up to %d characters):\n", MAX_CONTENT - 1);
    if (fgets(input_buffer, MAX_CONTENT, stdin) == NULL) {
        printf("Error reading message content.\n");
        return;
    }
    
    // Remove trailing newline
    input_buffer[strcspn(input_buffer, "\n")] = '\0';
    
    // Get exclusive access to write
    wait_sem(sem_io_id);
    
    // Increment index and check bounds
    wait_sem(sem_idx_id);
    (*in_idx)++;
    if (*in_idx >= MAX_MESSAGES) {
        printf("Message buffer full. Cannot send more messages.\n");
        (*in_idx)--;
        signal_sem(sem_idx_id);
        signal_sem(sem_io_id);
        return;
    }
    
    // Write message to shared memory
    messages[*in_idx].sender_id = client_id;
    messages[*in_idx].num_recv = num_recipients;
    
    // Copy recipients
    for (int i = 0; i < num_recipients; i++) {
        messages[*in_idx].receivers[i] = recipients[i];
    }
    if (num_recipients < MAX_RECEIVERS) {
        messages[*in_idx].receivers[num_recipients] = -1;  // Mark end of recipients
    }
    
    // Copy content
    strncpy(messages[*in_idx].content, input_buffer, MAX_CONTENT - 1);
    messages[*in_idx].content[MAX_CONTENT - 1] = '\0';  // Ensure null termination
    
    signal_sem(sem_idx_id);
    signal_sem(sem_io_id);
    
    printf("Message sent successfully!\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <client_id>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // Get client ID from command line argument
    client_id = atoi(argv[1]);
    
    // Initialize keys
    init_keys();
    
    // Get the semaphore IDs
    sem_io_id = semget(sem_io_key, 1, 0);
    sem_read_id = semget(read_sem_key, 1, 0);
    sem_idx_id = semget(sem_idx_key, 1, 0);
    
    if (sem_io_id == -1 || sem_read_id == -1 || sem_idx_id == -1) {
        perror("Failed to get semaphores");
        exit(EXIT_FAILURE);
    }
    
    // Attach to shared memory segments
    int messages_id = shmget(messages_key, MAX_MESSAGES * sizeof(struct message), 0);
    int num_readers_id = shmget(num_readers_key, sizeof(int), 0);
    int in_idx_id = shmget(in_idx_key, sizeof(int), 0);
    
    if (messages_id == -1 || num_readers_id == -1 || in_idx_id == -1) {
        perror("Failed to get shared memory segments");
        exit(EXIT_FAILURE);
    }
    
    messages = (struct message *)shmat(messages_id, NULL, 0);
    num_readers = (int *)shmat(num_readers_id, NULL, 0);
    in_idx = (int *)shmat(in_idx_id, NULL, 0);
    
    if (messages == (void *)-1 || num_readers == (void *)-1 || in_idx == (void *)-1) {
        perror("Failed to attach to shared memory");
        exit(EXIT_FAILURE);
    }
    
    printf("Chat client %d started\n", client_id);
    
    // Main loop
    char choice;
    while (1) {
        // Check for new messages
        reader();
        
        printf("Chat Client %d\n", client_id);
        printf("Send a new message? [y/N/e]: ");
        scanf(" %c", &choice);
        
        // Clear input buffer
        while (getchar() != '\n');
        
        if (choice == 'y' || choice == 'Y') {
            send_message();
        } else if (choice == 'e' || choice == 'E') {
            printf("Exiting chat client %d...\n", client_id);
            break;
        }
        
        // Wait before next cycle
        sleep(1);
    }
    
    // Detach from shared memory
    shmdt(messages);
    shmdt(num_readers);
    shmdt(in_idx);
    
    return 0;
}