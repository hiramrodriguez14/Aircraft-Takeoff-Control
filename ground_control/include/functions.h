#ifndef INCLUDE_FUNCTIONS_H_
#define INCLUDE_FUNCTIONS_H_
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

// Global Macros
#define PLANES_LIMIT 20
#define SHM_NAME "/control_shm" //Shared memory name
void MemoryCreate();
void SSR_SIGTERM(int signal);
void SSR_SIGUSR1(int signal);
void Traffic(int signum);

//  Global Variables
extern int planes;
extern int takeoffs;
extern int total_takeoffs;
extern int *shm_ptr;

/* timer is defined and initialized in a .c file to avoid executable statements in a header */
extern struct itimerval timer;
#endif //  INCLUDE_FUNCTIONS_H_
