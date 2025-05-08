# Chat-OS: Multi-Client Chat System

A robust, real-time chat system built using System V IPC mechanisms on Linux. This system demonstrates inter-process communication in a multi-client environment using shared memory and semaphores.

## Features

- **Multi-Client Support**: Create and manage multiple chat clients, each running in its own terminal
- **Real-Time Messaging**: Send and receive messages instantly between connected clients
- **Targeted Messaging**: Send messages to specific recipients rather than broadcasting to all clients
- **Reader-Priority Access**: Implements a readers-writer pattern that favors readers, allowing multiple clients to read simultaneously
- **Resource Management**: Automatic cleanup of all IPC resources when the system terminates

## System Architecture

The Chat-OS system consists of two main components:

1. **Parent Process (chat-os)**: 
   - Initializes shared memory segments and semaphores
   - Creates and manages client processes
   - Handles cleanup of resources

2. **Client Processes (child)**: 
   - Connects to shared resources
   - Provides user interface for sending/receiving messages
   - Implements synchronization for shared memory access

## Shared Resources

- **Messages Array**: Shared memory segment storing all messages
- **in_idx**: Shared counter tracking the last valid message
- **num_readers**: Shared counter tracking active readers
- **Semaphores**:
  - `sem_io`: Controls access to the message array
  - `read_sem`: Manages reader count access
  - `sem_idx`: Protects access to the in_idx variable

## Getting Started

### Prerequisites

- Linux operating system
- GCC compiler
- Make utility
- xterm terminal emulator

### Building the System

```bash
make
```

### Running the Chat System

```bash
./chat-os
```

When prompted, enter the number of chat clients you want to create. The system will open a separate xterm window for each client.

### Using the Chat Clients

In each client terminal:

1. You'll automatically see any new messages addressed to you
2. You'll be prompted with: "Send a new message? [y/N/e]"
   - Enter 'y' to send a message
   - Enter 'e' to exit the client
   - Press any other key to continue checking for new messages

3. When sending a message:
   - Enter the number of recipients
   - Enter each recipient's ID
   - Type your message content

### Cleaning Up

To remove compiled binaries:

```bash
make clean
```

## Implementation Details

- **Message Structure**:
  ```c
  struct message {
      int sender_id;
      int receivers[MAX_RECEIVERS];  // End marked by -1
      int num_recv;
      char content[MAX_CONTENT];
  };
  ```

- **Synchronization**: Uses a classic reader-priority pattern that allows multiple simultaneous readers but exclusive access for writers

- **Memory Management**: All IPC resources are properly created, managed, and cleaned up when the system terminates

This project demonstrates the use of System V IPC mechanisms for inter-process communication.