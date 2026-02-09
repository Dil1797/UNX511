CC=g++
CFLAGS=-Wall
LIBS=-lrt -lpthread

all: client1 client2 client3

client1: client1.cpp client.h
	$(CC) $(CFLAGS) $< -o $@ $(LIBS)

client2: client2.cpp client.h
	$(CC) $(CFLAGS) $< -o $@ $(LIBS)

client3: client3.cpp client.h
	$(CC) $(CFLAGS) $< -o $@ $(LIBS)

clean:
	rm -f *.o client1 client2 client3
