CC = gcc
CFLAGS = -g -Wall -Wextra -pedantic -std=c99

all: master client

master: master.c
	$(CC) $(CFLAGS) master.c master_client.c -o master.o -pthread

client: client.c
	$(CC) $(CFLAGS) client.c master_client.c -o client.o

clean:
	rm -f *.o *.fifo
