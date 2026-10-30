#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <pthread.h>

struct ThreadArgs {
  long long begin;
  long long end;
  long long mod;
};

// Общий результат и мьютекс для его защиты
long long total_result = 1;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

void *ThreadFactorial(void *args) {
  struct ThreadArgs *fargs = (struct ThreadArgs *)args;
  long long local_res = 1;

  // Локальный подсчет части факториала без блокировок
  for (long long i = fargs->begin; i <= fargs->end; i++) {
    local_res = ((__int128)local_res * (i % fargs->mod)) % fargs->mod;
  }

  // Критическая секция: объединяем локальный результат с глобальным
  pthread_mutex_lock(&mut);
  total_result = ((__int128)total_result * local_res) % fargs->mod;
  pthread_mutex_unlock(&mut);

  return NULL;
}

int main(int argc, char **argv) {
  long long k = -1;
  int pnum = -1;
  long long mod = -1;

  while (true) {
    static struct option options[] = {
      {"k", required_argument, 0, 'k'},
      {"pnum", required_argument, 0, 0},
      {"mod", required_argument, 0, 0},
      {0, 0, 0, 0}
    };

    int option_index = 0;
    int c = getopt_long(argc, argv, "k:", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 'k':
        k = atoll(optarg);
        break;
      case 0:
        switch (option_index) {
          case 0:
            k = atoll(optarg);
            break;
          case 1:
            pnum = atoi(optarg);
            break;
          case 2:
            mod = atoll(optarg);
            break;
        }
        break;
      case '?':
      default:
        printf("Unknown argument\n");
        return 1;
    }
  }

  if (k <= 0 || pnum <= 0 || mod <= 0) {
    printf("Usage: %s -k 10 --pnum=4 --mod=10\n", argv[0]);
    return 1;
  }

  // Если потоков запрошено больше, чем чисел, ограничиваем их
  if (pnum > k) {
    pnum = (int)k;
  }

  pthread_t threads[pnum];
  struct ThreadArgs args[pnum];

  long long chunk_size = k / pnum;

  for (int i = 0; i < pnum; i++) {
    args[i].begin = i * chunk_size + 1;
    args[i].end = (i == pnum - 1) ? k : (i + 1) * chunk_size;
    args[i].mod = mod;

    if (pthread_create(&threads[i], NULL, ThreadFactorial, (void *)&args[i]) != 0) {
      perror("pthread_create failed");
      return 1;
    }
  }

  for (int i = 0; i < pnum; i++) {
    pthread_join(threads[i], NULL);
  }

  printf("Factorial (%lld!) mod %lld = %lld\n", k, mod, total_result);
  return 0;
}
