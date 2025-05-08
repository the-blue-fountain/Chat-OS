CC = gcc
CFLAGS = -Wall -Wextra -g

all: chat-os child

chat-os: chat-os.c globals.h
	$(CC) $(CFLAGS) -o chat-os chat-os.c

child: child.c globals.h
	$(CC) $(CFLAGS) -o child child.c

clean:
	rm -f chat-os child

.PHONY: all clean