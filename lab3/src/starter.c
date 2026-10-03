#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
  pid_t pid = fork();

  if (pid < 0) {
    perror("fork failed");
    return 1;
  } else if (pid == 0) {
    // Дочерний процесс: заменяем образ процесса на sequential_min_max
    char *args[] = {"./sequential_min_max", "42", "1000", NULL};
    execv("./sequential_min_max", args);

    // Если execv сработал успешно, этот код никогда не выполнится
    perror("execv failed");
    exit(1);
  } else {
    // Родительский процесс ждёт окончания работы дочернего
    wait(NULL);
    printf("Sequential min max finished via exec!\n");
  }

  return 0;
}
