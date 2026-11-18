#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr

int main(int argc, char *argv[]) {
  const size_t kSize = sizeof(struct sockaddr_in);

  if (argc < 3) {
    printf("Usage: %s <PORT> <BUFSIZE>\n", argv[0]);
    exit(1);
  }

  int port = atoi(argv[1]);
  int bufsize = atoi(argv[2]);
  if (bufsize <= 0) {
    fprintf(stderr, "Invalid buffer size\n");
    exit(1);
  }

  char *buf = malloc(bufsize);
  if (!buf) {
    perror("malloc");
    exit(1);
  }

  int lfd, cfd;
  int nread;
  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;

  if ((lfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    free(buf);
    exit(1);
  }

  int opt = 1;
  setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  memset(&servaddr, 0, kSize);
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);

  if (bind(lfd, (SADDR *)&servaddr, kSize) < 0) {
    perror("bind");
    free(buf);
    close(lfd);
    exit(1);
  }

  if (listen(lfd, 5) < 0) {
    perror("listen");
    free(buf);
    close(lfd);
    exit(1);
  }

  printf("TCP Server listening on port %d with bufsize %d...\n", port, bufsize);

  while (1) {
    unsigned int clilen = kSize;

    if ((cfd = accept(lfd, (SADDR *)&cliaddr, &clilen)) < 0) {
      perror("accept");
      free(buf);
      close(lfd);
      exit(1);
    }
    printf("\n[TCP] Connection established!\n");

    while ((nread = read(cfd, buf, bufsize)) > 0) {
      write(1, buf, nread);
    }

    if (nread == -1) {
      perror("read");
    }
    printf("[TCP] Client disconnected.\n");
    close(cfd);
  }

  free(buf);
  close(lfd);
  return 0;
}
