// writer.c
#include <fcntl.h>  // For O_* constants
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>  // For sleep function

#define SHM_SIZE 1024             ///< Size of shared memory in bytes
#define SEM_PRODUCER "/producer"  ///< Name of the producer semaphore
#define SEM_CONSUMER "/consumer"  ///< Name of the consumer semaphore

/**
 * @brief Main function to write data to shared memory.
 *
 * This program creates and initializes semaphores for synchronization with
 * a consumer process. It creates a shared memory segment and writes data
 * into it in a loop, signaling the consumer process when new data is
 * available. The producer and consumer use semaphores to synchronize their
 * access to the shared memory.
 *
 * The program performs the following steps:
 * 1. Unlinks any existing semaphores with the same names.
 * 2. Creates and initializes the producer and consumer semaphores.
 * 3. Creates a shared memory segment and attaches to it.
 * 4. Writes data to shared memory in a loop.
 * 5. Uses semaphores to synchronize access between producer and consumer.
 * 6. Detaches from shared memory and closes semaphores upon completion.
 *
 * @return int Exit status of the program. Returns 0 on successful completion.
 */
int main() {
  key_t key;   ///< Unique key for the shared memory segment.
  int shmid;   ///< Shared memory segment ID.
  char *data;  ///< Pointer to the shared memory segment.
  sem_t *sem_producer,
      *sem_consumer;  ///< Semaphore handles for producer and consumer.

  // Unlink semaphores before creation to avoid errors if they already exist.
  sem_unlink(SEM_PRODUCER);
  sem_unlink(SEM_CONSUMER);

  // Create and initialize semaphores
  sem_producer = sem_open(SEM_PRODUCER, O_CREAT, 0644, 1);
  sem_consumer = sem_open(SEM_CONSUMER, O_CREAT, 0644, 0);

  if (sem_producer == SEM_FAILED || sem_consumer == SEM_FAILED) {
    perror("sem_open");
    exit(EXIT_FAILURE);
  }

  // Create a unique key for the shared memory segment.
  key = ftok("/tmp", 65);  // Ensure "/tmp" exists and is accessible.
  if (key == -1) {
    perror("ftok");
    exit(EXIT_FAILURE);
  }

  // Create a shared memory segment with read and write permissions.
  shmid = shmget(key, SHM_SIZE, 0666 | IPC_CREAT);
  if (shmid == -1) {
    perror("shmget failed");
    exit(EXIT_FAILURE);
  }

  // Attach to the shared memory segment.
  data = (char *)shmat(shmid, NULL, 0);
  if (data == (char *)-1) {
    perror("shmat failed");
    exit(EXIT_FAILURE);
  }

  printf("Address of the block: %p\n", (void *)data);

  // Write to shared memory in a loop.
  for (int i = 0; i < 1000000; i++) {
    // Wait for the consumer to be ready by waiting on the producer semaphore.
    if (sem_wait(sem_producer) == -1) {
      perror("sem_wait failed");
      exit(EXIT_FAILURE);
    }

    // Write an integer value to shared memory.
    snprintf(data, SHM_SIZE, "%d", i + 1);
    printf("Written: %s\n", data);

    // Signal the consumer that new data is available by posting to the consumer
    // semaphore.
    if (sem_post(sem_consumer) == -1) {
      perror("sem_post failed");
      exit(EXIT_FAILURE);
    }
  }

  // Detach from the shared memory segment.
  shmdt(data);

  // Close and unlink semaphores.
  sem_close(sem_producer);
  sem_close(sem_consumer);

  return 0;
}
