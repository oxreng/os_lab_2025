#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <getopt.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#include "multmodulo.h"

struct FactorialArgs {
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
};

uint64_t Factorial(const struct FactorialArgs *args) {
  uint64_t ans = 1;
  for (uint64_t i = args->begin; i <= args->end; i++) {
    ans = MultModulo(ans, i, args->mod);
  }
  return ans;
}

void *ThreadFactorial(void *args) {
  struct FactorialArgs *fargs = (struct FactorialArgs *)args;
  uint64_t ans = Factorial(fargs);
  return (void *)(uintptr_t)ans;
}

int main(int argc, char **argv) {
  int tnum = -1;
  int port = -1;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"port", required_argument, 0, 0},
                                      {"tnum", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1) break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        port = atoi(optarg);
        if (port <= 0) {
          fprintf(stderr, "port must be positive\n");
          return 1;
        }
        break;
      case 1:
        tnum = atoi(optarg);
        if (tnum <= 0) {
          fprintf(stderr, "tnum must be positive\n");
          return 1;
        }
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Unknown argument\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (port == -1 || tnum == -1) {
    fprintf(stderr, "Using: %s --port 20001 --tnum 4\n", argv[0]);
    return 1;
  }

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("socket failed");
    return 1;
  }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons((uint16_t)port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);

  int opt_val = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

  int err = bind(server_fd, (struct sockaddr *)&server, sizeof(server));
  if (err < 0) {
    perror("bind failed");
    return 1;
  }

  err = listen(server_fd, 128);
  if (err < 0) {
    perror("listen failed");
    return 1;
  }

  printf("Server listening on port %d with %d threads...\n", port, tnum);

  while (true) {
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);

    if (client_fd < 0) {
      perror("accept failed");
      continue;
    }

    while (true) {
      unsigned int buffer_size = sizeof(uint64_t) * 3;
      char from_client[buffer_size];
      int read_bytes = recv(client_fd, from_client, buffer_size, 0);

      if (read_bytes == 0) break;
      if (read_bytes < 0) {
        perror("recv failed");
        break;
      }
      if (read_bytes < (int)buffer_size) {
        fprintf(stderr, "Client sent wrong data format\n");
        break;
      }

      uint64_t begin = 0, end = 0, mod = 0;
      memcpy(&begin, from_client, sizeof(uint64_t));
      memcpy(&end, from_client + sizeof(uint64_t), sizeof(uint64_t));
      memcpy(&mod, from_client + 2 * sizeof(uint64_t), sizeof(uint64_t));

      printf("Task received: [%llu, %llu] mod %llu\n", (unsigned long long)begin, (unsigned long long)end, (unsigned long long)mod);

      // Распараллеливание внутри сервера
      int actual_threads = tnum;
      uint64_t total_numbers = end - begin + 1;
      if (total_numbers < (uint64_t)actual_threads) {
        actual_threads = total_numbers;
      }

      pthread_t threads[actual_threads];
      struct FactorialArgs args[actual_threads];
      uint64_t chunk = total_numbers / actual_threads;

      for (int i = 0; i < actual_threads; i++) {
        args[i].begin = begin + i * chunk;
        args[i].end = (i == actual_threads - 1) ? end : (begin + (i + 1) * chunk - 1);
        args[i].mod = mod;

        if (pthread_create(&threads[i], NULL, ThreadFactorial, (void *)&args[i])) {
          perror("pthread_create failed");
          return 1;
        }
      }

      uint64_t total = 1;
      for (int i = 0; i < actual_threads; i++) {
        void *thread_result;
        pthread_join(threads[i], &thread_result);
        uint64_t res = (uint64_t)(uintptr_t)thread_result;
        total = MultModulo(total, res, mod);
      }

      printf("Subtask result: %llu\n", (unsigned long long)total);

      char buffer[sizeof(total)];
      memcpy(buffer, &total, sizeof(total));
      err = send(client_fd, buffer, sizeof(total), 0);
      if (err < 0) {
        perror("send failed");
        break;
      }
    }

    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
  }

  close(server_fd);
  return 0;
}
