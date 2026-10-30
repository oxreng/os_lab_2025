#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
  pid_t pid = fork();

  if (pid < 0) {
    perror("fork failed");
    return 1;
  }

  if (pid == 0) {
    // Дочерний процесс завершается мгновенно
    printf("[Child] PID: %d. Exiting...\n", getpid());
    exit(0);
  } else {
    // Родитель НЕ вызывает wait(), а просто спит 20 секунд
    printf("[Parent] PID: %d. Created child with PID: %d\n", getpid(), pid);
    printf("[Parent] Sleeping for 20 seconds. Child is now a ZOMBIE!\n");
    sleep(20);
    printf("[Parent] Exiting. Child adopted by init/systemd and reaped.\n");
  }

  return 0;
}
