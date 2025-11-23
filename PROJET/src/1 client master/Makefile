CC = gcc
CFLAGS = -g -Wall -Wextra -pedantic -std=c99

all: master client

master: master.c
	$(CC) $(CFLAGS) master.c master_client.c -o master.o

client: client.c
	$(CC) $(CFLAGS) client.c master_client.c -oclient.o

clean:
	rm -f *.o *.fifo
