#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <semaphore.h>

// CreateSemaphore : This function creates a shared memory
//                   segment for an uninitialized semaphore.
//                   Adress of start of this memory segment is returned.
sem_t *CreateSemaphore(void);

// P : Same specification as sem_post();
void P(sem_t *semaphore);

// V : Same specification as sem_wait();
void V(sem_t *semaphore);

// init : Same specification as sem_init();
void init(sem_t *semaphore, unsigned int value);

// destroy : Same specification as sem_destroy();
void destroy(sem_t *sempahore);

