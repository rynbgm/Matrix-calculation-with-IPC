#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define NOMBRE_DE_SPAM 30

int main(void) {

  for (int c = 0; c < NOMBRE_DE_SPAM; c++) {
    pid_t pid = fork();
    if (pid < 0) {
      perror("fork");
      exit(EXIT_FAILURE);
    }
    if (pid == 0) {
      execlp("./client", "./client", "7", "9", "4", "400", NULL);
      perror("execlp");
      exit(EXIT_FAILURE);
    }
  }
  for (int c = 0; c < NOMBRE_DE_SPAM; c++) {
    wait(NULL);
  }
  return EXIT_SUCCESS;
}
