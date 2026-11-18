CC=gcc
CFLAGS=-Wall -pthread -I.

all: libmultmodulo.a server client

libmultmodulo.a: multmodulo.o
	ar rcs libmultmodulo.a multmodulo.o

multmodulo.o: multmodulo.c multmodulo.h
	$(CC) $(CFLAGS) -c multmodulo.c

server: server.c libmultmodulo.a
	$(CC) $(CFLAGS) -o server server.c -L. -lmultmodulo

client: client.c libmultmodulo.a
	$(CC) $(CFLAGS) -o client client.c -L. -lmultmodulo

clean:
	rm -f *.o *.a server client servers.txt
