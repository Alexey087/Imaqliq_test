CFLAGS += -Wall -std=gnu99 -pedantic
TARGETS = client server_daemon
CC = gcc

all: $(TARGETS)

client:
	@echo "Creating client..."
	$(CC) $(CFLAGS) -o client common.c client.c

server_daemon:
	@echo "Creating server_daemon..."
	$(CC) $(CFLAGS) -o server_daemon common.c server_daemon.c

clean:
	$(RM) $(TARGETS)

.PHONY: clean

