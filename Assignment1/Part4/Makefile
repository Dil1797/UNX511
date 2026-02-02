CC = gcc
CFLAGS = -Wall -g -std=gnu99

all: parent_main worker_process

parent_main: parent_main.o dhsb_logger.o
	$(CC) $(CFLAGS) -o parent_main parent_main.o dhsb_logger.o

worker_process: worker_process.c
	$(CC) $(CFLAGS) -o worker_process worker_process.c

parent_main.o: parent_main.c
	$(CC) $(CFLAGS) -c parent_main.c

dhsb_logger.o: dhsb_logger.c
	$(CC) $(CFLAGS) -c dhsb_logger.c

clean:
	rm -f *.o parent_main worker_process syslog.log

run: all
	./parent_main
