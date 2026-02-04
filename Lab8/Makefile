CC=g++
CFLAGS=-std=c++11 -Wall
LIBS=-lpthread

all: server client1 client2 client3

server: server.cpp
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

client1: client1.cpp
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

client2: client2.cpp
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

client3: client3.cpp
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

clean:
	rm -f *.o server client1 client2 client3
