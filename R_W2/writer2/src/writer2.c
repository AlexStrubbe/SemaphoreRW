// writer.c
#include <fcntl.h> // For O_* constants
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h> // For sleep function

#define SHM_SIZE 1024 // Size of shared memory in bytes
#define SEM_PRODUCER "/producer"
#define SEM_CONSUMER "/consumer"

int main()
{
  key_t key;
  int shmid;
  char *data;
  sem_t *sem_producer, *sem_consumer;

  // Unlink semaphores before creation
  sem_unlink(SEM_PRODUCER);
  sem_unlink(SEM_CONSUMER);

  // Create and initialize semaphores
  sem_producer = sem_open(SEM_PRODUCER, O_CREAT, 0644, 1);
  sem_consumer = sem_open(SEM_CONSUMER, O_CREAT, 0644, 0);

  if (sem_producer == SEM_FAILED || sem_consumer == SEM_FAILED)
  {
    perror("sem_open");
    exit(EXIT_FAILURE);
  }

  // Create a unique key for the shared memory
  key = ftok("/tmp", 65); // Ensure "/tmp" exists and is accessible
  if (key == -1)
  {
    perror("ftok");
    exit(EXIT_FAILURE);
  }

  // Create a shared memory segment
  shmid = shmget(key, SHM_SIZE, 0666 | IPC_CREAT);
  if (shmid == -1)
  {
    perror("shmget failed");
    exit(EXIT_FAILURE);
  }

  // Attach to the shared memory segment
  data = (char *)shmat(shmid, NULL, 0);
  if (data == (char *)-1)
  {
    perror("shmat failed");
    exit(EXIT_FAILURE);
  }

  printf("Address of the block: %p\n", (void *)data);

  // take starting time
  int t = time(NULL);

  // Write to shared memory
  for (int i = 0; i <= 10; i++)
  {
    // Wait for the consumer to be ready
    if (sem_wait(sem_producer) == -1)
    {
      perror("sem_wait failed");
      exit(EXIT_FAILURE);
    }

    if (i == 0)
    {
      snprintf(data, SHM_SIZE, "%ln", &t);
      if (sem_post(sem_consumer) == -1)
      {
        perror("sem_post failed");
        exit(EXIT_FAILURE);
      }
    }
    else
    {
      // Write to shared memory
      snprintf(data, SHM_SIZE, "%d", i);
      printf("Written: %s\n", data);
    }
    // Signal the consumer that data is available
    if (sem_post(sem_consumer) == -1)
    {
      perror("sem_post failed");
      exit(EXIT_FAILURE);
    }
  }

  // Detach from shared memory
  shmdt(data);
  // Close and unlink semaphores
  sem_close(sem_producer);
  sem_close(sem_consumer);

  return 0;
}