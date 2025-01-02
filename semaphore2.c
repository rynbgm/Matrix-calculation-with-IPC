#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <semaphore.h>

sem_t *CreateSemaphore(void) {
    sem_t *semaphore = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE,
                            MAP_SHARED | 32, -1, 0);

    if (semaphore == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    return semaphore;
}

void P(sem_t *semaphore) {
	if(sem_wait(semaphore) == -1) {
		perror("sem_wait");
		exit(EXIT_FAILURE);
	}
}

void V(sem_t *semaphore) {
	if(sem_post(semaphore) == -1) {
		perror("sem_post");
		exit(EXIT_FAILURE);
	}
}


void init(sem_t *semaphore, unsigned int i) {
	if(sem_init(semaphore,1,i) == -1) {
		perror("sem_init");
		exit(EXIT_FAILURE);
	}
}


void destroy(sem_t *semaphore) {
	if(sem_destroy(semaphore) == -1) {
		perror("sem_destroy");
		exit(EXIT_FAILURE);
	}
}
