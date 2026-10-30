CC=gcc
CFLAGS=-I.

all: parallel_min_max process_memory zombie psum

parallel_min_max: utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)

process_memory: process_memory.c
	$(CC) -w -o process_memory process_memory.c

zombie: zombie.c
	$(CC) -o zombie zombie.c

libsum.a: sum.o
	ar rcs libsum.a sum.o

sum.o: sum.c sum.h
	$(CC) -o sum.o -c sum.c $(CFLAGS)

psum: parallel_sum.c utils.o libsum.a
	$(CC) -o psum parallel_sum.c utils.o -L. -lsum -lpthread $(CFLAGS)

utils.o: utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

find_min_max.o: utils.h find_min_max.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

clean:
	rm -f utils.o find_min_max.o sum.o libsum.a parallel_min_max process_memory zombie psum
