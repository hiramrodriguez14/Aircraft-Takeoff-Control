#include "./../include/functions.h"
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
  // TODO:
  // 1. Open the shared memory block and store this process PID in position 2
  //    of the memory block.
  MemoryCreate();
  shm_ptr[2] = getpid(); // Store ground_control PID in position 2

  // 3. Configure SIGTERM and SIGUSR1 handlers
  //    - The SIGTERM handler should: close the shared memory, print
  //      "finalization of operations..." and terminate the program.
  //    - The SIGUSR1 handler should: increase the number of takeoffs by 5.

  struct sigaction sa1, sa2;
  sa1.sa_handler = SSR_SIGTERM;
  sa2.sa_handler = SSR_SIGUSR1;
  sigaction(SIGTERM, &sa1, NULL);
  sigaction(SIGUSR1, &sa2, NULL);
  // 2. Configure the timer to execute the Traffic function.
  struct sigaction sa3;
  sa3.sa_handler = Traffic;
  sigaction(SIGALRM, &sa3, NULL);
  setitimer(ITIMER_REAL, &timer, NULL); //500ms interval timer

 while(running){
    pause(); // Wait for signals
  }
  
  return 0;
}
