#include "./../include/functions.h"

#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int planes = 0;
int takeoffs = 0;
int* shm_ptr = NULL;  // For ground_control shared memory access
bool running = true;
struct itimerval timer = {
  .it_value = { .tv_sec = 0, .tv_usec = 500000 }, // 500ms initial delay
  .it_interval = { .tv_sec = 0, .tv_usec = 500000 } // 500ms interval
};
void Traffic(int signum) {
  // TODO:
  // Calculate the number of waiting planes.
  int waiting_planes = planes - takeoffs;
  printf("Planes: %d\n", planes);
  if(waiting_planes >= 10){
    printf("RUNWAY OVERLOADED, waiting planes in line... %d\n", waiting_planes);
  }
  if(planes < PLANES_LIMIT){
    planes+=5;
    kill(shm_ptr[1], SIGUSR2);
  }
  // Check if there are 10 or more waiting planes to send a signal and increment
  // planes. Ensure signals are sent and planes are incremented only if the
  // total number of planes has not been exceeded.
}
void SSR_SIGTERM(int signal) {
  // Close the shared memory
  printf("finalization of operations...\n");
  shm_unlink(SHM_NAME);
  running = false;
}

void SSR_SIGUSR1(int signal) {
  // Increase the number of takeoffs by 5.
  takeoffs += 5;
}

void MemoryCreate() {
  // TODO2: create the shared memory segment, configure it and store the PID of
  // the process in the first position of the memory block.

  // Create shared memory segment and set its size to hold 3 integers
  int fd = shm_open(SHM_NAME,O_RDWR, 0666);
  ftruncate(fd, 3 * sizeof(int));
  shm_ptr = mmap(0, 3 * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  // Check for mmap failure
  if (shm_ptr == MAP_FAILED) {
    perror("Ground control mmap failed");
    exit(1);
  }

}
