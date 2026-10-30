#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

void *ThreadFunc1(void *arg) {
  printf("[Thread 1] Trying to lock mutex1...\n");
  pthread_mutex_lock(&mutex1);
  printf("[Thread 1] Locked mutex1. Sleeping 1 second...\n");
  sleep(1);

  printf("[Thread 1] Trying to lock mutex2...\n");
  pthread_mutex_lock(&mutex2); // Зависнет здесь!
  printf("[Thread 1] Successfully locked mutex2!\n");

  pthread_mutex_unlock(&mutex2);
  pthread_mutex_unlock(&mutex1);
  return NULL;
}

void *ThreadFunc2(void *arg) {
  printf("[Thread 2] Trying to lock mutex2...\n");
  pthread_mutex_lock(&mutex2);
  printf("[Thread 2] Locked mutex2. Sleeping 1 second...\n");
  sleep(1);

  printf("[Thread 2] Trying to lock mutex1...\n");
  pthread_mutex_lock(&mutex1); // Зависнет здесь!
  printf("[Thread 2] Successfully locked mutex1!\n");

  pthread_mutex_unlock(&mutex1);
  pthread_mutex_unlock(&mutex2);
  return NULL;
}

int main() {
  pthread_t t1, t2;

  printf("=== Deadlock Demonstration Started ===\n");
  pthread_create(&t1, NULL, ThreadFunc1, NULL);
  pthread_create(&t2, NULL, ThreadFunc2, NULL);

  pthread_join(t1, NULL);
  pthread_join(t2, NULL);

  printf("=== Finished (you will never see this line) ===\n");
  return 0;
}
