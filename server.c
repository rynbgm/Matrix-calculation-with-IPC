#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif
#define _POSIX_C_SOURCE 199309L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/random.h>
#include <signal.h>
#include <errno.h>
#include <stdint.h>
#include "semaphore2.h"

#define DEMON "demon"
#define SHM "/SHMSHM"
#define PID_NAME_MAX_LENGTH 12

#define TRACK() printf("[TRACK] File: %s, Line: %d\n", __FILE__, __LINE__)

typedef struct attr attr;
struct attr {
  int N;
  int M;
  int i;
  int line;
  int P_;
  volatile int **zone;
};

typedef struct request request;
struct request {
  char client_pid[PID_NAME_MAX_LENGTH];
  int max_value;
  int n;
  int m;
  int p;
};

// Transforme le processus courrant en démon.
void make_deamon();

// Cette fonction effectue la gestion des signaux pour le serveur,
// SIGINT, SIGTERM, SIGCHLD.
int signaux_general();

// Rend le serveur insensible aux interuptions.
void interuption_non(int sig);

// Vaccine le serveur contre les zombies.
void no_zomb(int signum);

// Routine de calcul pour les threads du WorkerB
// Calcule de la matrice A * B
void *calc(void *arg);

int main() {

  // Le serveur devient un démon.
  make_deamon();
  if (signaux_general() == 1) {
    exit(EXIT_FAILURE);
  }


  // Création du tube géneral du serveur
  if (mkfifo(DEMON, 0666) == -1 && errno != EEXIST) {
    perror("mkfifo");
    exit(EXIT_FAILURE);
  }


  int fd;
  fd = open(DEMON, O_RDONLY);
  if (fd == -1) {
    perror("open");
    exit(EXIT_FAILURE);
  }


  // Le serveur
  while (1) {
    request req;

    ssize_t bytes_read = read(fd, &req, sizeof req);
    if (bytes_read == 0) {
      int fd_b = open(DEMON, O_WRONLY);
      if (fd_b == -1) {
        perror("open");
        exit(EXIT_FAILURE);
      }
    }

    int N = req.n;
    int M = req.m;
    int P_ = req.p;
    int MAX = req.max_value;

    if (bytes_read == -1) {
      if (errno == EINTR) {
        continue;
      } else {
        perror("read");
        break;
      }
    }

    if (bytes_read > 0) {

      pid_t pid = fork();
      if (pid == -1) {
        perror("fork");
        break;
      }

      if (pid == 0) {

        if (close(fd) == -1) {
          perror("close");
          exit(EXIT_FAILURE);
        }

        int fd_rep = open(req.client_pid, O_WRONLY, 0777);
        if (fd_rep == -1) {
          perror("open");
          exit(EXIT_FAILURE);
        }

        int shm_fd;
        size_t taille = (size_t) (N * M + M * P_ + N * P_);

        if ((shm_fd = shm_open(SHM, O_RDWR | O_CREAT, 0777)) == -1) {
          perror("shmopen");
          exit(EXIT_FAILURE);
        }

        if (ftruncate(shm_fd, (off_t) (taille * sizeof(int))) == -1) {
          perror("ftruncate");
          close(shm_fd);
          shm_unlink(SHM);
          exit(EXIT_FAILURE);
        }

        volatile int *zone = mmap(NULL, taille * sizeof(int),
            PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED,
            shm_fd, 0);

        if (zone == MAP_FAILED) {
          perror("mmap");
          close(shm_fd);
          shm_unlink(SHM);
          exit(EXIT_FAILURE);
        }

        sem_t *rdv = CreateSemaphore();
        init(rdv, 0);

        if (close(shm_fd) == -1) {
            perror("close");
            exit(EXIT_FAILURE);
        }

        switch (fork()) {

          case -1:
            perror("fork");
            exit(EXIT_FAILURE);

          case 0:
            //worker B
            for (int i = N * M; i < M * N + M * P_; i++) {
              srand((unsigned int) getpid() + (unsigned int) i);
              zone[i] = (rand()) % (MAX + 1);
            }

            P(rdv);

            int line = 0;
            for (int i = 0; i < N * P_ && line < N; ++i) {
              if (i != 0 && i % P_ == 0) {
                ++line;
              }
              pthread_t th;
              attr *atr = malloc(sizeof *atr);
              atr->N = N;
              atr->P_ = P_;
              atr->i = i;
              atr->M = M;
              atr->zone = &zone;
              atr->line = line;
              // Création du thread
              if (pthread_create(&th, NULL, calc, atr) != 0) {
                perror("pthread_detach");
                unlink(SHM);
                exit(EXIT_FAILURE);
              }
              if (pthread_detach(th) != 0) {
                perror("pthread_detach");
                unlink(SHM);
                exit(EXIT_FAILURE);
              }
            }

            exit(EXIT_SUCCESS);

          default:
            // Création de la Matrice A
            for (int i = 0; i < M * N; i++) {
              srand((unsigned int) getpid() + (unsigned int) i);
              zone[i] = (rand()) % (MAX + 1);
            }
            // Worker B attend que Worker A crée sa matrice avant
            // de calculer A * B.

            V(rdv);
            wait(NULL);

            int *rep = malloc(taille * sizeof(int));
            if (rep == NULL) {
              perror("malloc");
              exit(EXIT_FAILURE);
            }

            for (size_t i = 0; i < taille; ++i) {
              rep[i] = zone[i];
            }

            size_t s = (taille * sizeof(int));
            ssize_t d = 0;
            while ((d = write(fd_rep, rep, s)) != (ssize_t) s) {
              if (d == -1) {
                perror("write");
                exit(EXIT_FAILURE);
              }
              s = s - (size_t) d;
            }

            if (close(fd_rep) == -1) {
              perror("close");
              exit(EXIT_FAILURE);
            }

            if (shm_unlink(SHM) == -1) {
              perror("shm_unlink");
              exit(EXIT_FAILURE);
            }

            exit(EXIT_SUCCESS);
            break;
        }

        break;

        if (pid > 0) {
          int r;
          do {
            r = waitpid(-1, NULL, WNOHANG);
          } while (r > 0);
        }
      }
    }
  }

  if (close(fd) == -1) {
    perror("close");
    exit(EXIT_FAILURE);
  }

  if (unlink(DEMON) == -1) {
    perror("unlink");
    exit(EXIT_FAILURE);
  }

  return EXIT_SUCCESS;
}

void make_deamon() {
  pid_t p = fork();
  if (p == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
  }
  if (p > 0) {
    exit(EXIT_SUCCESS);
  }
  if (setsid() == (pid_t) -1) {
    perror("setsid");
    exit(EXIT_FAILURE);
  }
  if (signal(SIGHUP, SIG_IGN) == SIG_ERR) {
    perror("signal");
    exit(EXIT_FAILURE);
  }
  if (close(STDIN_FILENO) == -1 || close(STDOUT_FILENO) == -1
      || close(STDERR_FILENO) == -1) {
    perror("close");
    exit(EXIT_FAILURE);
  }
  int fds = open("/dev/null", O_RDWR);
  if (fds != -1) {
    dup2(fds, STDIN_FILENO);
    dup2(fds, STDOUT_FILENO);
    dup2(fds, STDERR_FILENO);
    if (fds > STDERR_FILENO) {
      if (close(fds) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
      }
    }
  }
  if (close(fds) == -1) {
    perror("close");
    exit(EXIT_FAILURE);
  }
  umask(0);
}

void *calc(void *arg) {
  attr *atr = (attr *) arg;
  int N = atr->N;
  int P_ = atr->P_;
  int i = atr->i;
  int M = atr->M;
  volatile int *zone = *(atr->zone);
  int line = atr->line;
  int somme = 0; // Réinitialiser la somme pour le calcul de C[line][col]
  for (int k = 0; k < M; ++k) {
    somme += zone[line * M + k] * zone[N * M + k * P_ + (i % P_)];
  }
  zone[N * M + M * P_ + i] = somme;
  return NULL;
}

void interuption_non(int sig) {
  if (sig == SIGINT || sig == SIGTERM) {
    return;
  }
}

void no_zomb(int signum) {
  if (signum != SIGCHLD) {
    exit(EXIT_FAILURE);
  }
  int errnold = errno;
  int r;
  do {
    r = waitpid(-1, NULL, WNOHANG);
  } while (r > 0);
  if (r == -1) {
    if (errno != ECHILD) {
      perror("waitpid");
      exit(EXIT_FAILURE);
    }
  }
  errno = errnold;
}

int signaux_general() {
  int r = 0;
  struct sigaction action2;
  action2.sa_handler = interuption_non;
  sigemptyset(&action2.sa_mask);
  action2.sa_flags = 0;
  if (sigfillset(&action2.sa_mask) == -1) {
    perror("sigfillset");
    exit(EXIT_FAILURE);
  }
  if (sigaction(SIGINT, &action2, NULL) == -1) {
    r = 1;
  }
  if (sigaction(SIGTERM, &action2, NULL) == -1) {
    r = 1;
  }
  struct sigaction action;
  action.sa_handler = no_zomb;
  action.sa_flags = 0;
  if (sigfillset(&action.sa_mask) == -1) {
    r = 1;
  }
  if (sigaction(SIGCHLD, &action, NULL) == -1) {
    r = 1;
  }
  return r;
}

