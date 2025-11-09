#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include "functions.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#define TOTAL_TAKEOFFS 20
// Defining Global Variables
int planes = 0;
int takeoffs = 0;
int total_takeoffs = 0;
int fd = 0;
int done = 0;

int *shm_ptr;   //Global pointer to shared memory

// Defining Global Mutex variables
pthread_mutex_t state_lock;  
pthread_mutex_t runway1_lock ;  
pthread_mutex_t runway2_lock;  



void MemoryCreate() {
  // TODO2: create the shared memory segment, configure it and store the PID of
  // the process in the first position of the memory block.
  
  // Create shared memory segment and set its size to hold 3 integers
  shm_unlink(SHM_NAME); // Ensure previous shm is removed
  fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  if (fd == -1) {
    perror("shm_open failed");
    exit(1);
  }
  ftruncate(fd, 3 * sizeof(int));
  shm_ptr = mmap(0, 3 * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  // Check for mmap failure
  if (shm_ptr == MAP_FAILED) {
    perror("Air control mmap failed");
    exit(1);
  }

  shm_ptr[0] = getpid(); // Storing air_control PID in the first position
}

void SigHandler2(int signal) {
  //pthread_mutex_lock(&state_lock);
  planes += 5;
  //pthread_mutex_unlock(&state_lock);
}

void* TakeOffsFunction() {
  // TODO: implement the logic to control a takeoff thread
  //    Use a loop that runs while total_takeoffs < TOTAL_TAKEOFFS
  //    Use runway1_lock or runway2_lock to simulate a locked runway
  //    Use state_lock for safe access to shared variables (planes,
  //    takeoffs, total_takeoffs)
  //    Simulate the time a takeoff takes with sleep(1)
  //    Send SIGUSR1 every 5 local takeoffs
  //    Send SIGTERM when the total takeoffs target is reached

  while (done == 0) {
    
    //printf("Total takeoffs: %d\n", total_takeoffs);
    int result = pthread_mutex_trylock(&runway1_lock);
    int result2 = (result != 0) ? pthread_mutex_trylock(&runway2_lock) : 1;

    if (result == 0) {
      pthread_mutex_lock(&state_lock);
      if (planes > 0) {
        planes--;
        takeoffs++;
        total_takeoffs++;
        if (takeoffs == 5) {
          kill(shm_ptr[1], SIGUSR1);
          takeoffs = 0;
        }
      }
      pthread_mutex_unlock(&state_lock);
      sleep(1);  // Simulate takeoff time
      pthread_mutex_unlock(&runway1_lock);

      if (total_takeoffs >= TOTAL_TAKEOFFS) {
        done = 1;
      }
    } else if (result2 == 0) {
      pthread_mutex_lock(&state_lock);
      if (planes > 0) {
        planes--;
        takeoffs++;
        total_takeoffs++;
        if (takeoffs == 5) {
          kill(shm_ptr[1], SIGUSR1);
          takeoffs = 0;
        }
        sleep(1);
      }
      pthread_mutex_unlock(&state_lock);
      //sleep(1);  // Simulate takeoff time
      pthread_mutex_unlock(&runway2_lock);
      if (total_takeoffs >= TOTAL_TAKEOFFS) {
        done = 1;
      }
    } else {
      usleep(10000);
    }
  }
  kill(shm_ptr[1], SIGTERM);
  close(fd);
  return NULL;
}
