CC = g++
CFLAGS = -Wall -std=c++11
LIBS = -lpthread

# Targets
all: client1 client2 client3 server

client1: client.cpp
	$(CC) $(CFLAGS) client.cpp -o client1 $(LIBS)

client2: client.cpp
	$(CC) $(CFLAGS) client.cpp -o client2 $(LIBS)

client3: client.cpp
	$(CC) $(CFLAGS) client.cpp -o client3 $(LIBS)

server: server.cpp
	$(CC) $(CFLAGS) server.cpp -o server $(LIBS)

clean:
	rm -f *.o client1 client2 client3 server
