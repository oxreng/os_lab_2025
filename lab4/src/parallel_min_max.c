#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

static bool timeout_happened = false;

void SigAlarmHandler(int sig) {
  timeout_happened = true;
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout = -1;
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {"timeout", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
              printf("seed must be positive\n");
              return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
              printf("array_size must be positive\n");
              return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
              printf("pnum must be positive\n");
              return 1;
            }
            break;
          case 3:
            with_files = true;
            break;
          case 4:
            timeout = atoi(optarg);
            if (timeout <= 0) {
              printf("timeout must be positive\n");
              return 1;
            }
            break;
          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;
      case '?':
        break;
      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"num\"] [--by_files]\n",
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  pid_t *child_pids = malloc(sizeof(pid_t) * pnum);

  int (*pipes)[2] = NULL;
  if (!with_files) {
    pipes = malloc(sizeof(int[2]) * pnum);
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipes[i]) < 0) {
        perror("pipe failed");
        return 1;
      }
    }
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  int chunk_size = array_size / pnum;

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      active_child_processes += 1;
      if (child_pid == 0) {
        // Дочерний процесс
        unsigned int begin = i * chunk_size;
        unsigned int end = (i == pnum - 1) ? array_size : (i + 1) * chunk_size;

        struct MinMax local_min_max = GetMinMax(array, begin, end);

        if (with_files) {
          char filename[64];
          sprintf(filename, "result_%d.txt", i);
          FILE *fp = fopen(filename, "w");
          if (fp) {
            fprintf(fp, "%d %d\n", local_min_max.min, local_min_max.max);
            fclose(fp);
          }
        } else {
          close(pipes[i][0]);
          write(pipes[i][1], &local_min_max, sizeof(struct MinMax));
          close(pipes[i][1]);
        }
        free(array);
        free(child_pids);
        if (!with_files) free(pipes);
        return 0;
      } else {
        child_pids[i] = child_pid;
      }
    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  // Настройка таймаута при помощи сигналов
  if (timeout > 0) {
    signal(SIGALRM, SigAlarmHandler);
    alarm(timeout);
  }

  // Ожидание детей
  while (active_child_processes > 0) {
    if (timeout_happened) {
      printf("\n[TIMEOUT] Time limit (%d sec) exceeded! Sending SIGKILL to children...\n", timeout);
      for (int i = 0; i < pnum; i++) {
        kill(child_pids[i], SIGKILL);
      }
      // Собираем завершенные процессы
      while (active_child_processes > 0) {
        waitpid(-1, NULL, 0);
        active_child_processes--;
      }
      printf("All children terminated by timeout.\n");
      free(array);
      free(child_pids);
      if (!with_files) free(pipes);
      return 1;
    }

    if (timeout > 0) {
      // Неблокирующее ожидание при заданном таймауте
      pid_t pid = waitpid(-1, NULL, WNOHANG);
      if (pid > 0) {
        active_child_processes--;
      } else {
        usleep(10000); // 10 мс пауза, чтобы не нагружать процессор
      }
    } else {
      // Обычное блокирующее ожидание, если таймаут не задан
      wait(NULL);
      active_child_processes--;
    }
  }

  // Отключаем таймер, если процессы успели завершиться вовремя
  if (timeout > 0) {
    alarm(0);
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      char filename[64];
      sprintf(filename, "result_%d.txt", i);
      FILE *fp = fopen(filename, "r");
      if (fp) {
        fscanf(fp, "%d %d", &min, &max);
        fclose(fp);
        remove(filename);
      }
    } else {
      close(pipes[i][1]);
      struct MinMax local_res;
      read(pipes[i][0], &local_res, sizeof(struct MinMax));
      close(pipes[i][0]);
      min = local_res.min;
      max = local_res.max;
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  free(child_pids);
  if (!with_files) free(pipes);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}
