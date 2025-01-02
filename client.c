#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "semaphore2.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define DEMON "demon"
#define PID_NAME_MAX_LENGTH 12
#define TIME_OUT_VALUE 15

typedef struct request request;

struct request {
  char client_pid[PID_NAME_MAX_LENGTH];
  int max_value;
  int n;
  int m;
  int p;
};

char *pid_file = NULL;

// Mise en place du timeout
void time();

// Gestion du Timeout
void timeout(int sig);

// Gestion de SIGINT
int inter();
void interuption(int sig);

// Affiche les matrices A B et C
void affiche_matrices(request *req, long int taille, int *reponse);

// Utilise strtol pour convertir val en nombre, -1 en cas d'erreur,
// 0 sinon.
int tol(char *val);

// Affiche l'aide
void help();

int main(int argc, char **argv) {

  if (argc == 2) {
    if (strcmp("-h", argv[1]) == 0 || strcmp("--help", argv[1]) == 0) {
      help();
      exit(EXIT_SUCCESS);
    }
  }

  if (argc != 5) {
    fprintf(stderr, "usage...\n");
    exit(EXIT_FAILURE);
  }

  if (inter() == 1) {
    exit(EXIT_FAILURE);
  }

  // Ouverture du tube avec le serveur.
  int fd = open(DEMON, O_WRONLY);
  if (fd == -1) {
    perror("open");
    exit(EXIT_FAILURE);
  }

  request *req = malloc(sizeof(request));
  if (req == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  char pid_n[PID_NAME_MAX_LENGTH];
  snprintf(pid_n, sizeof(pid_n), "%d", getpid());
  pid_file = pid_n;
  req->n = tol(argv[1]);
  req->m = tol(argv[2]);
  req->p = tol(argv[3]);
  req->max_value = tol(argv[4]);
  if (req->n == -1 || req->m == -1 || req->p == -1 || req->max_value == -1) {
    fprintf(stderr, "Invalid Argument..\n");
    exit(EXIT_FAILURE);
  }
  strcpy(req->client_pid, pid_n);

  // Création du tube réponse entre Worker A et le client.
  if (mkfifo(pid_n, 0666) == -1) {
    perror("mkfifo");
    exit(EXIT_FAILURE);
  }

  // Envoie de la requête au serveur.
  if (write(fd, req, sizeof(request)) == -1) {
    perror("write");
    close(fd);
    free(req);
    exit(EXIT_FAILURE);
  }

  if (close(fd) == -1) {
    perror("close");
    exit(EXIT_FAILURE);
  }

  // Ouverture du pipe entre le client et Worker A
  int fd_rep = open(pid_n, O_RDONLY);
  if (fd_rep == -1) {
    perror("open");
    unlink(pid_n);
    exit(EXIT_FAILURE);
  }

  // Taille des matrices A B et C
  long int taille = (req->n) * (req->m) + (req->m) * (req->p) + (req->n)
      * (req->p);
  int *reponse = malloc((size_t) taille * sizeof(int));
  if (reponse == NULL) {
    perror("malloc");
    close(fd_rep);
    exit(EXIT_FAILURE);
  }

  // Lecture de la réponse du Worker A.
  time();
  size_t s = (size_t) taille * sizeof(int);
  ssize_t d = 0;
  int *ptr = reponse;

  while (s > 0) {
    d = read(fd_rep, ptr, s);
    if (d == -1) {
      perror("read");
      free(reponse);
      close(fd_rep);
      exit(EXIT_FAILURE);
    }
    s -= (size_t) d;
    ptr += (size_t) d / sizeof(int);
  }
  alarm(0);

  if (unlink(pid_n) == -1) {
    perror("unlink");
    exit(EXIT_FAILURE);
  }

  if (close(fd_rep) == -1) {
    perror("close");
    exit(EXIT_FAILURE);
  }

  affiche_matrices(req, taille, reponse);
  free(reponse);
  free(req);

  return EXIT_SUCCESS;
}

void time() {
  signal(SIGALRM, timeout);
  alarm(TIME_OUT_VALUE);
}

int inter() {
  struct sigaction action;
  action.sa_handler = interuption;
  if (sigemptyset(&action.sa_mask) == -1) {
    perror("sigemptyset");
    return 1;
  }
  action.sa_flags = 0;
  if (sigaction(SIGINT, &action, NULL) == -1) {
    perror("sigaction");
    return 1;
  }
  return 0;
}

void interuption(int sig) {
  if (sig == SIGINT) {
    if (pid_file != NULL) {
      if (unlink(pid_file) == -1) {
        perror("unlink");
      }
    }
    exit(EXIT_SUCCESS);
  }
}

void timeout(int sig) {
  if (sig) {
    // inutilisé.
  }
  printf("Le serveur ne répond pas.\n");
  if (unlink(pid_file) == -1) {
    perror("unlink");
  }
  exit(EXIT_FAILURE);
}

void affiche_matrices(request *req, long int taille, int *reponse) {
  printf("Voici la matrice A : \n");
  for (int i = 0; i < req->n * req->m; ++i) {
    if (i != 0 && i % req->n == 0) {
      printf("\n");
    }
    printf("%d ", reponse[i]);
  }
  printf("\n");
  printf("\n");
  printf("Voici la matrice B : \n");
  for (int i = req->n * req->m; i < req->n * req->m + req->m * req->p; ++i) {
    if (i != req->n * req->m && i % req->n == 0) {
      printf("\n");
    }
    printf("%d ", reponse[i]);
  }
  printf("\n");
  printf("\n");
  printf("Voici la matrice C  : \n");
  for (int i = req->n * req->m + req->m * req->p; i < taille; ++i) {
    if (i != req->n * req->m + req->m * req->p && i % req->n == 0) {
      printf("\n");
    }
    printf("%d ", reponse[i]);
  }
  printf("\n");
}

int tol(char *val) {
  errno = 0;
  char *endptr;
  long value = strtol(val, &endptr, 10);
  if (errno != 0) {
    return -1;
  }
  if (*endptr != '\0') {
    return -1;
  } else {
    return (int) value;
  }
}

void help() {
  printf("\nApplication Calcul de deux matrices\n\n");
  printf("Usage: ./client [OPTIONS]\n\n");
  printf("Options:\n\n");
  printf("-h, --help\n\n\tAffiche ce message d'aide.\n\n");
  printf("./client N M P MAX_VALUE\n\n\tTransmet une requête de calcul\n");
  printf(
      "\tà un serveur. Celui si calcul le produit d'une matrice A de taille M * N\n");
  printf("\tpar une matrice B de taille M * P dont les valeurs sont générées\n");
  printf(
      "\taléatoirement par le serveur, valeurs comprisent entre 0 et MAX_VALUE.\n");
  printf(
      "\tEnfin affiche à l'écran les matrice A et B ainsi que leurs produit.\n\n");
}
