CC=gcc
CFLAGS=-pthread -Wall

all: mutex_no_lock mutex_with_lock factorial deadlock

mutex_no_lock: mutex.c
	$(CC) $(CFLAGS) -o mutex_no_lock mutex.c

mutex_with_lock: mutex_with_lock.c
	$(CC) $(CFLAGS) -o mutex_with_lock mutex_with_lock.c

factorial: factorial.c
	$(CC) $(CFLAGS) -o factorial factorial.c

deadlock: deadlock.c
	$(CC) $(CFLAGS) -o deadlock deadlock.c

clean:
	rm -f mutex_no_lock mutex_with_lock factorial deadlock
