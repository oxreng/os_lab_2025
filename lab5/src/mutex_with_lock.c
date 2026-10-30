#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void do_one_thing(int *);
void do_another_thing(int *);
void do_wrap_up(int);
int common = 0;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

int main() {
  pthread_t thread1, thread2;

  if (pthread_create(&thread1, NULL, (void *)do_one_thing, (void *)&common) != 0) {
    perror("pthread_create");
    exit(1);
  }

  if (pthread_create(&thread2, NULL, (void *)do_another_thing, (void *)&common) != 0) {
    perror("pthread_create");
    exit(1);
  }

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);

  do_wrap_up(common);
  return 0;
}

void do_one_thing(int *pnum_times) {
  int i, work;
  unsigned long k;
  for (i = 0; i < 50; i++) {
    pthread_mutex_lock(&mut); // Захват мьютекса
    work = *pnum_times;
    work++;
    for (k = 0; k < 500000; k++);
    *pnum_times = work;
    pthread_mutex_unlock(&mut); // Освобождение мьютекса
  }
}

void do_another_thing(int *pnum_times) {
  int i, work;
  unsigned long k;
  for (i = 0; i < 50; i++) {
    pthread_mutex_lock(&mut); // Захват мьютекса
    work = *pnum_times;
    work++;
    for (k = 0; k < 500000; k++);
    *pnum_times = work;
    pthread_mutex_unlock(&mut); // Освобождение мьютекса
  }
}

void do_wrap_up(int counter) {
  printf("All done, counter = %d\n", counter);
}
