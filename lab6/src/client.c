#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#include "multmodulo.h"

struct Server {
  char ip[255];
  int port;
};

struct ClientThreadArgs {
  struct Server server;
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
  uint64_t result;
  bool success;
};

bool ConvertStringToUI64(const char *str, uint64_t *val) {
  char *end = NULL;
  unsigned long long i = strtoull(str, &end, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Out of uint64_t range: %s\n", str);
    return false;
  }
  if (errno != 0) return false;
  *val = i;
  return true;
}

void *ConnectAndCompute(void *args) {
  struct ClientThreadArgs *targs = (struct ClientThreadArgs *)args;
  targs->success = false;
  targs->result = 1;

  struct hostent *hostname = gethostbyname(targs->server.ip);
  if (hostname == NULL) {
    fprintf(stderr, "gethostbyname failed with %s\n", targs->server.ip);
    return NULL;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(targs->server.port);
  server_addr.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

  int sck = socket(AF_INET, SOCK_STREAM, 0);
  if (sck < 0) {
    perror("Socket creation failed");
    return NULL;
  }

  if (connect(sck, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("Connection to server failed");
    close(sck);
    return NULL;
  }

  char task[sizeof(uint64_t) * 3];
  memcpy(task, &targs->begin, sizeof(uint64_t));
  memcpy(task + sizeof(uint64_t), &targs->end, sizeof(uint64_t));
  memcpy(task + 2 * sizeof(uint64_t), &targs->mod, sizeof(uint64_t));

  if (send(sck, task, sizeof(task), 0) < 0) {
    perror("Send failed");
    close(sck);
    return NULL;
  }

  char response[sizeof(uint64_t)];
  if (recv(sck, response, sizeof(response), 0) < 0) {
    perror("Receive failed");
    close(sck);
    return NULL;
  }

  memcpy(&targs->result, response, sizeof(uint64_t));
  targs->success = true;

  close(sck);
  return NULL;
}

int main(int argc, char **argv) {
  uint64_t k = -1;
  uint64_t mod = -1;
  char servers_file[255] = {'\0'};

  while (true) {
    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1) break;

    switch (c) {
    case 0:
      switch (option_index) {
      case 0:
        if (!ConvertStringToUI64(optarg, &k)) return 1;
        break;
      case 1:
        if (!ConvertStringToUI64(optarg, &mod)) return 1;
        break;
      case 2:
        strncpy(servers_file, optarg, sizeof(servers_file) - 1);
        break;
      }
      break;
    default:
      fprintf(stderr, "Invalid options\n");
      return 1;
    }
  }

  if (k == -1 || mod == -1 || strlen(servers_file) == 0) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n", argv[0]);
    return 1;
  }

  // Читаем список серверов из файла
  FILE *fp = fopen(servers_file, "r");
  if (!fp) {
    perror("Cannot open servers file");
    return 1;
  }

  struct Server servers[64];
  int servers_num = 0;
  char line[256];

  while (fgets(line, sizeof(line), fp)) {
    char *colon = strchr(line, ':');
    if (colon) {
      *colon = '\0';
      strncpy(servers[servers_num].ip, line, sizeof(servers[servers_num].ip) - 1);
      servers[servers_num].port = atoi(colon + 1);
      servers_num++;
    }
  }
  fclose(fp);

  if (servers_num == 0) {
    fprintf(stderr, "No servers found in file\n");
    return 1;
  }

  printf("Connected to %d servers in parallel...\n", servers_num);

  pthread_t threads[servers_num];
  struct ClientThreadArgs targs[servers_num];

  uint64_t chunk = k / servers_num;

  // Запуск потоков для каждого сервера параллельно
  for (int i = 0; i < servers_num; i++) {
    targs[i].server = servers[i];
    targs[i].begin = i * chunk + 1;
    targs[i].end = (i == servers_num - 1) ? k : (i + 1) * chunk;
    targs[i].mod = mod;

    pthread_create(&threads[i], NULL, ConnectAndCompute, (void *)&targs[i]);
  }

  uint64_t total_answer = 1;

  for (int i = 0; i < servers_num; i++) {
    pthread_join(threads[i], NULL);
    if (!targs[i].success) {
      fprintf(stderr, "Server %s:%d failed to compute!\n", servers[i].ip, servers[i].port);
      return 1;
    }
    total_answer = MultModulo(total_answer, targs[i].result, mod);
  }

  printf("Final answer: %llu\n", (unsigned long long)total_answer);
  return 0;
}
