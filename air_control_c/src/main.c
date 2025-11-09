#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/wait.h>
#include "functions.h"

int main() {
  // TODO 1: Call the function that creates the shared memory segment.
  MemoryCreate();
  // TODO 3: Configure the SIGUSR2 signal to increment the planes on the runway
  // by 5.
  struct sigaction sa;
  sa.sa_handler = SigHandler2;
  sigaction(SIGUSR2, &sa, NULL);
  // TODO 4: Launch the 'radio' executable and, once launched, store its PID in
  // the second position of the shared memory block.
  pid_t child;
  child = fork();
  if (child == 0) {
    execlp("./../radio/build/radio", "./radio", SHM_NAME, NULL);
    perror("execlp failed");
    exit(1);
  } else if (child > 0) {
    shm_ptr[1] = child;
  } else {
    perror("fork failed");
    exit(1);
  }
  // TODO 6: Launch 5 threads which will be the controllers; each thread will
  // execute the TakeOffsFunction().
  pthread_t thread1, thread2, thread3, thread4, thread5;

  pthread_mutex_init(&state_lock, NULL);
  pthread_mutex_init(&runway1_lock, NULL);
  pthread_mutex_init(&runway2_lock, NULL);

  pthread_create(&thread1, NULL, TakeOffsFunction, NULL);
  pthread_create(&thread2, NULL, TakeOffsFunction, NULL);
  pthread_create(&thread3, NULL, TakeOffsFunction, NULL);
  pthread_create(&thread4, NULL, TakeOffsFunction, NULL);
  pthread_create(&thread5, NULL, TakeOffsFunction, NULL);

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);
  pthread_join(thread3, NULL);
  pthread_join(thread4, NULL);
  pthread_join(thread5, NULL);

  wait(NULL);
  munmap(shm_ptr, 3 * sizeof(int));
  shm_unlink(SHM_NAME);
  return 0;
}
