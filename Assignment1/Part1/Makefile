# Makefile for Part 1 - Binary File Generator
CC = gcc
CFLAGS = -Wall -g -std=c99
TARGET = file_generator
SRC = file_generator.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) *.bin
