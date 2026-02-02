CC = gcc
CFLAGS = -Wall -g -std=c99

all: test_worker_main

worker_process.o: worker_process.c
	$(CC) $(CFLAGS) -c worker_process.c -o worker_process.o

test_worker_main.o: test_worker_main.c
	$(CC) $(CFLAGS) -c test_worker_main.c -o test_worker_main.o

test_worker_main: test_worker_main.o worker_process.o
	$(CC) $(CFLAGS) test_worker_main.o worker_process.o -o test_worker_main

run: test_worker_main
	./test_worker_main

clean:
	rm -f *.o test_worker_main
