# Makefile for Part 2 - Worker Process Testing

CC = gcc
CFLAGS = -Wall -g -std=c99 -I.

# Targets
WORKER_TARGET = worker_process
TEST_TARGET = test_worker_main

# Source files
WORKER_SRC = worker_process.c
TEST_SRC = test_worker_main.c $(WORKER_SRC)

# Default target: build worker_process.o (not standalone executable)
all: $(WORKER_TARGET)

$(WORKER_TARGET): $(WORKER_SRC)
	$(CC) $(CFLAGS) -c $(WORKER_SRC) -o $(WORKER_TARGET).o

# Build a full test program to run the worker
test: $(TEST_SRC)
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $(TEST_SRC)

# Run the test binary
run: test
	./$(TEST_TARGET)

# Clean up all binaries
clean:
	rm -f $(WORKER_TARGET) $(WORKER_TARGET).o $(TEST_TARGET)
