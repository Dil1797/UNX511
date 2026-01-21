CC = gcc
CFLAGS = -Wall -Wextra
TARGET = memcheck

all: $(TARGET)

$(TARGET): memcheck.c process_monitor.h
	$(CC) $(CFLAGS) memcheck.c -o $(TARGET)
clean:
	rm -f $(TARGET)
