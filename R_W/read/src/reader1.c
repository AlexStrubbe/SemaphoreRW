#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h> // For sleep function
#include <time.h>   // For time functions

#define SHM_SIZE 1024 ///< Size of shared memory in bytes

#define SEM_PRODUCER "/producer" ///< Name of the producer semaphore
#define SEM_CONSUMER "/consumer" ///< Name of the consumer semaphore

/**
 * @brief Main function to read data from shared memory.
 *
 * This program opens existing semaphores used for synchronization with
 * a producer process. It attaches to an existing shared memory segment,
 * reads data from it, and maintains a counter to sum the values read.
 * It measures and reports the total time taken to process data. The program
 * performs the following steps:
 * 1. Opens the producer and consumer semaphores.
 * 2. Gets the shared memory segment and attaches to it.
 * 3. Reads data from shared memory in a loop, updating the counter and
 *    printing the values.
 * 4. Waits for the producer to signal availability of new data.
 * 5. Cleans up shared memory and semaphores after completion.
 *
 * @return int Exit status of the program. Returns 0 on successful completion.
 */
int main()
{
  key_t key;                          ///< Unique key for the shared memory segment.
  int shmid;                          ///< Shared memory segment ID.
  long counter = 0;                   ///< Counter to accumulate the sum of values read.
  char *data;                         ///< Pointer to the shared memory segment.
  sem_t *sem_producer, *sem_consumer; ///< Semaphore handles for producer and consumer.
  time_t start_time, end_time;        ///< Variables to measure processing time.

  // Open existing semaphores for synchronization.
  sem_producer = sem_open(SEM_PRODUCER, 0);
  sem_consumer = sem_open(SEM_CONSUMER, 0);

  if (sem_producer == SEM_FAILED || sem_consumer == SEM_FAILED)
  {
    perror("sem_open");
    exit(EXIT_FAILURE);
  }

  // Create a unique key for the shared memory segment.
  key = ftok("/tmp", 65);
  if (key == -1)
  {
    perror("ftok");
    return 1;
  }

  // Get the shared memory segment.
  shmid = shmget(key, SHM_SIZE, 0666);
  if (shmid == -1)
  {
    perror("shmget failed");
    exit(EXIT_FAILURE);
  }

  // Attach to the shared memory segment.
  data = (char *)shmat(shmid, NULL, 0);
  if (data == (char *)-1)
  {
    perror("shmat failed");
    exit(EXIT_FAILURE);
  }

  printf("Address of the block: %p\n", (void *)data);

  // Start timing the data processing.
  start_time = time(NULL);

  while (1)
  {
    // Wait for the producer to signal that data is available.
    sem_wait(sem_consumer);
    int value = atoi(data);

    // Check if data is available.
    if (strlen(data) > 0)
    {
      counter += value;
      printf("Read value: %d\n", value);
      data[0] = '\0'; // Clear shared memory after processing.
    }

    // Signal the producer that space is available in shared memory.
    if (sem_post(sem_producer) == -1)
    {
      perror("sem_post failed");
      exit(EXIT_FAILURE);
    }

    // Exit the loop if the value read is 1000000.
    if (value == 1000000)
      break;
  }

  // Stop timing and print results.
  end_time = time(NULL);
  printf("Final Counter: %ld\n", counter);
  printf("Total Time: %ld seconds\n", end_time - start_time);

  // Detach from the shared memory segment.
  if (shmdt(data) == -1)
  {
    perror("shmdt failed");
  }

  // Destroy the shared memory segment.
  shmctl(shmid, IPC_RMID, NULL);

  // Close semaphores.
  sem_close(sem_producer);
  sem_close(sem_consumer);

  return 0;
}
