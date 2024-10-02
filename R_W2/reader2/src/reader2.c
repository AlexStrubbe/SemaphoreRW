#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h> // For sleep function
#include <time.h>   // For time functions

#define SHM_SIZE 1024 // Shared memory size

#define SEM_PRODUCER "/producer"
#define SEM_CONSUMER "/consumer"

int main()
{
  key_t key;
  int shmid;
  long counter = 0;
  char *data;
  sem_t *sem_producer, *sem_consumer;
  time_t start_time, end_time;

  // sem_unlink(SEM_PRODUCER);
  // sem_unlink(SEM_CONSUMER);

  // Open existing semaphores
  sem_producer = sem_open(SEM_PRODUCER, 0);
  sem_consumer = sem_open(SEM_CONSUMER, 0);

  if (sem_producer == SEM_FAILED || sem_consumer == SEM_FAILED)
  {
    perror("sem_open");
    exit(EXIT_FAILURE);
  }

  key = ftok("/tmp", 65);
  if (key == -1)
  {
    perror("ftok");
    return 1;
  }

  // Get the shared memory segment
  shmid = shmget(key, SHM_SIZE, 0666);
  if (shmid == -1)
  {
    perror("shmget failed");
    exit(EXIT_FAILURE);
  }

  // Attach the shared memory segment
  data = (char *)shmat(shmid, NULL, 0);
  if (data == (char *)-1)
  {
    perror("shmat failed");
    exit(EXIT_FAILURE);
  }

  printf("Address of the block: %p\n", (void *)data);
  while (1)
  {
    sem_wait(sem_consumer);
    int value = atoi(data);

    // Check if data is available
    if (counter == 0)
    {

      start_time = atol(data);
      data[0] = '\0'; // Clear shared memory after processing
      counter++;
    }
    else
    {
      if (strlen(data) > 0)
      {
        counter += value;
        printf("Read value: %d\n", value);
        data[0] = '\0'; // Clear shared memory after processing
      }

      // Check if the counter has reached the target value

      if (sem_post(sem_producer) == -1)
      {
        perror("sem_post failed");
        exit(EXIT_FAILURE);
      }
      if (value == 10)
      {
        printf("%d", start_time);
        break;
      }
    }
  }

  // Stop timing

  end_time = time(NULL);
  printf("Final Counter: %ld\n", counter);
  printf("Total Time: %ld seconds\n", end_time - start_time);

  // Detach from shared memory
  if (shmdt(data) == -1)
  {
    perror("shmdt failed");
  }

  // Destroy the shared memory segment
  shmctl(shmid, IPC_RMID, NULL);

  // Close semaphores
  sem_close(sem_producer);
  sem_close(sem_consumer);

  return 0;
}