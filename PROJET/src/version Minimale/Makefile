CC = gcc
CFLAGS = -g -Wall -Wextra -pedantic -std=c99

all: worker.o master.o client.o

worker.o: worker.c
	$(CC) $(CFLAGS) worker.c -o worker.o

master.o: master.c
	$(CC) $(CFLAGS) master.c -o master.o

client.o: client.c
	$(CC) $(CFLAGS) client.c -o client.o

clean:
	rm -f *.o
