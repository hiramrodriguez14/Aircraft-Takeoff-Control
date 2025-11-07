#ifndef INCLUDE_FUNCTIONS_H_
#define INCLUDE_FUNCTIONS_H_
#include <pthread.h>

// Global Macros
#define SHM_NAME "/control_shm" //Shared memory name
#define TOTAL_TAKEOFFS 50      //Total takeoffs to be managed

//  Global Variables
extern int planes;
extern int takeoffs;
extern int total_takeoffs;
extern int *shm_ptr;

//Global Mutex variables
extern pthread_mutex_t state_lock;
extern pthread_mutex_t runway1_lock ;
extern pthread_mutex_t runway2_lock;

//Global Functions
void MemoryCreate();
void SigHandler2(int signal);
void* TakeOffsFunction();

#endif  //  INCLUDE_FUNCTIONS_H_
