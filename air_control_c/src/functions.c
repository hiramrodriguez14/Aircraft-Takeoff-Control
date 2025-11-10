#include "functions.h"

#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

// Defining Global Variables
int planes = 0;
int takeoffs = 0;
int total_takeoffs = 0;

int* shm_ptr;  // Global pointer to shared memory

// Defining Global Mutex variables
pthread_mutex_t state_lock;
pthread_mutex_t runway1_lock;
pthread_mutex_t runway2_lock;

void MemoryCreate() {
  // TODO2: create the shared memory segment, configure it and store the PID of
  // the process in the first position of the memory block.

  // Create shared memory segment and set its size to hold 3 integers
  shm_unlink(SHM_NAME);  // Ensure previous shm is removed
  int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
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

  shm_ptr[0] = getpid();  // Storing air_control PID in the first position
}

void SigHandler2(int signal) { planes += 5; }

void* TakeOffsFunction() {
  // Implement the logic to control a takeoff thread
  //    Use a loop that runs while total_takeoffs < TOTAL_TAKEOFFS
  //    Use runway1_lock or runway2_lock to simulate a locked runway
  //    Use state_lock for safe access to shared variables (planes,
  //    takeoffs, total_takeoffs)
  //    Simulate the time a takeoff takes with sleep(1)
  //    Send SIGUSR1 every 5 local takeoffs
  //    Send SIGTERM when the total takeoffs target is reached
  int64_t agent_id = (int64_t)pthread_self();  // Unique ID for the thread
  int local_takeoffs = 0;

  while (1) {
    // Check if total takeoffs goal reached
    pthread_mutex_lock(&state_lock);
    if (total_takeoffs >= TOTAL_TAKEOFFS) {
      pthread_mutex_unlock(&state_lock);
      break;
    }
    pthread_mutex_unlock(&state_lock);

    // Try to acquire a runway
    int used_runway = 0;
    if (pthread_mutex_trylock(&runway1_lock) == 0) {
      used_runway = 1;
    } else if (pthread_mutex_trylock(&runway2_lock) == 0) {
      used_runway = 2;
    } else {
      // No runway available → small delay and retry
      usleep(1000);  // 1 ms
      continue;
    }

    // Safely access shared state
    pthread_mutex_lock(&state_lock);

    if (total_takeoffs >= TOTAL_TAKEOFFS) {
      pthread_mutex_unlock(&state_lock);
      if (used_runway == 1)
        pthread_mutex_unlock(&runway1_lock);
      else
        pthread_mutex_unlock(&runway2_lock);
      break;
    }

    if (planes > 0) {
      planes--;
      takeoffs++;
      total_takeoffs++;
      local_takeoffs++;

     // printf("[Controller %lld] Plane taking off on runway %d (Total: %d)\n",
            // agent_id, used_runway, total_takeoffs);

      // Send SIGUSR1 every 5 local takeoffs
      if (takeoffs == 5) {
        if (shm_ptr[1] > 0) {
          kill(shm_ptr[1], SIGUSR1);
         // printf("[Controller %lld] Sent SIGUSR1 to radio.\n", agent_id);
        }
        takeoffs = 0;
      }
    }

    pthread_mutex_unlock(&state_lock);

    // Simulate time for takeoff
    sleep(1);

    // Release runway
    if (used_runway == 1)
      pthread_mutex_unlock(&runway1_lock);
    else
      pthread_mutex_unlock(&runway2_lock);
  }

  // Send SIGTERM when done
  if (shm_ptr[1] > 0) {
    kill(shm_ptr[1], SIGTERM);
    //printf("[Controller %lld] Sent SIGTERM to radio (finished).\n", agent_id);
  }

  return NULL;
}
