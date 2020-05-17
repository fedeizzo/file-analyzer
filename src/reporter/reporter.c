#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../list/list.h"
#include "../wrapping/wrapping.h"
#include "./reporter.h"

#define OK 0
#define MAXLEN 4096

#define NEW_DIRECTIVES 1

#define CAST_FAILURE -2
#define SEND_FAILURE -3

#define WRITE_CHANNEL 1
#define READ_CHANNEL 0

UserInput newUserInput() {
  UserInput ui = malloc(sizeof(struct UserInputStr));
  int rc_al = checkAllocationError(ui);
  int rc_al2 = OK;

  if (rc_al == OK) {
    ui->paths = newList();
    if (ui->paths == NULL)
      rc_al2 = -1;
    ui->managers = 3;
    ui->workers = 4;
  }

  if (rc_al < OK || rc_al2 < OK)
    ui = NULL;

  return ui;
}

void *userInputLoop(void *ptr);
int readDirectives(List paths, int *numManager, int *numWorker);

void *writeFifoLoop(void *ptr);
int sendDirectives(int fd, char *path, int *numManager, int *numWorker);
int readResult(char *fifo);

int main() {
  List paths;
  int managers;
  int workers;
  char *writeFifo = "/tmp/reporterToAnalyzer";
  char *readFifo = "/tmp/analyzerToReporter";
  /* remove(writeFifo); */
  int rc_fi = mkfifo(writeFifo, 0666);
  int rc_fi2 = mkfifo(readFifo, 0666);
  UserInput userInput;
  userInput = newUserInput();
  int iter1;
  int iter2;

  pthread_t userInputThraed;
  pthread_t writeFifoThraed;

  userInput_t input;
  pthread_mutex_init(&input.mutex, NULL);
  input.userInput = userInput;
  input.writeFifo = writeFifo;
  input.readFifo = readFifo;

  iter1 = pthread_create(&userInputThraed, NULL, userInputLoop, (void *)&input);
  iter2 = pthread_create(&writeFifoThraed, NULL, writeFifoLoop, (void *)&input);

  pthread_join(userInputThraed, NULL);
  pthread_join(writeFifoThraed, NULL);

  int rc_t = OK;

  return rc_t;
}

void *userInputLoop(void *ptr) {
  userInput_t *input = (userInput_t *)ptr;

  int rc_t = OK;
  while (rc_t == OK) {
    pthread_mutex_lock(&(input->mutex));
    rc_t =
        readDirectives(input->userInput->paths, &(input->userInput->managers),
                       &(input->userInput->workers));
    printf("numero di percorsi insertiti %d\n", input->userInput->paths->size);
    pthread_mutex_unlock(&(input->mutex));
    usleep(50000);
  }
}

void *writeFifoLoop(void *ptr) {
  userInput_t *input = (userInput_t *)ptr;

  int rc_t = OK;
  int fd;
  char *fifoPath = malloc(MAXLEN * sizeof(char));
  pthread_mutex_lock(&(input->mutex));
  strcpy(fifoPath, input->writeFifo);
  pthread_mutex_unlock(&(input->mutex));

  printf("sto aprendo la fifo %s\n", fifoPath);
  fd = open(fifoPath, O_WRONLY);
  printf("ho aprto la fifo\n");
  while (rc_t == OK) {
    pthread_mutex_lock(&(input->mutex));
    if (input->userInput->paths->size > 0) {
      char *path = front(input->userInput->paths);
      if (path != NULL) {
        int rc_po = pop(input->userInput->paths);
        rc_t = sendDirectives(fd, path, &(input->userInput->managers),
                              &(input->userInput->workers));
        printf("scritto nella fifo\n");
      }
      free(path);
    }
    pthread_mutex_unlock(&(input->mutex));
  }
  free(fifoPath);
}

int readDirectives(List paths, int *numManager, int *numWorker) {
  int rc_t = OK;
  char readBuffer[2] = "a";
  char *newPath = malloc(MAXLEN * sizeof(char));
  int rc_al = checkAllocationError(newPath);
  if (rc_al < OK) {
    rc_t = MALLOC_FAILURE;
  }
  char nWorker[MAXLEN];
  char nManager[MAXLEN];
  int counter = 0;
  int numberManager = 0;
  int numberWorker = 0;

  counter = 0;
  do {
    int rc = readChar(READ_CHANNEL, readBuffer);
    newPath[counter++] = readBuffer[0];
  } while (readBuffer[0] != '\0' && readBuffer[0] != '\n');
  newPath[counter] = '\0';
  if (newPath[strlen(newPath) - 1] == '\n') {
    newPath[strlen(newPath) - 1] = '\0';
  }
  counter = 0;
  do {
    int rc = readChar(READ_CHANNEL, readBuffer);
    nManager[counter++] = readBuffer[0];
  } while (readBuffer[0] != '\0' && readBuffer[0] != '\n');
  nManager[counter] = '\0';

  int rc_sc = sscanf(nManager, "%d", &numberManager);
  if (rc_sc == 0 || (numberManager == 9 && strcmp(nManager, "9") == 0)) {
    rc_t = CAST_FAILURE;
  }

  counter = 0;
  do {
    int rc = readChar(READ_CHANNEL, readBuffer);
    nWorker[counter++] = readBuffer[0];
  } while (readBuffer[0] != '\0' && readBuffer[0] != '\n');
  nWorker[counter] = '\0';

  int rc_sc2 = sscanf(nWorker, "%d", &numberWorker);
  if (rc_sc2 == 0 || (numberWorker == 9 && strcmp(nWorker, "9") == 0)) {
    rc_t = CAST_FAILURE;
  }

  if (newPath[0] == '\0' || nWorker[0] == '\0') {
    char *msgErr = (char *)malloc(300);
    int rc_ca = checkAllocationError(msgErr);
    if (rc_ca < 0) {
      printError("I can't allocate memory");
    } else {
      sprintf(msgErr, "inisde worker with pid: %d", getpid());
      printError(msgErr);
      free(msgErr);
    }
  } else if (rc_t == OK) {
    enqueue(paths, newPath);
    *numManager = numberManager;
    *numWorker = numberWorker;
    rc_t = NEW_DIRECTIVES;
  }

  free(newPath);

  return rc_t;
}

int sendDirectives(int fd, char *path, int *numManager, int *numWorker) {
  int rc_t = OK;

  char *nManager = malloc(MAXLEN * sizeof(char));
  int rc_al = checkAllocationError(nManager);
  char *nWorker = malloc(MAXLEN * sizeof(char));
  int rc_al2 = checkAllocationError(nWorker);

  int rc_sp = sprintf(nManager, "%d", *numManager);
  int rc_sp2 = sprintf(nWorker, "%d", *numWorker);

  if (rc_sp >= OK && rc_al2 >= OK && fd > OK) {
    printf("sono entrato qua e quindi ho invaito\n");
    printf("%s,%s,%s\n", path, nManager, nWorker);
    int rc_wr = writeDescriptor(fd, path);
    int rc_wr2 = writeDescriptor(fd, nManager);
    int rc_wr3 = writeDescriptor(fd, nWorker);
    int rc_wr4 = writeDescriptor(fd, "dire");

    if (rc_wr < OK || rc_wr2 < OK || rc_wr3 < OK)
      rc_t = SEND_FAILURE;
  } else
    rc_t = SEND_FAILURE;

  if (rc_al == OK)
    free(nManager);
  else
    rc_t = MALLOC_FAILURE;

  if (rc_al2 == OK)
    free(nWorker);
  else
    rc_t = MALLOC_FAILURE;

  if (fd < OK)
    rc_t = SEND_FAILURE;
  else
    closeDescriptor(fd);

  return rc_t;
}

int readResult(char *fifo) {
  int fd = open(fifo, O_RDONLY);
  char *dst = malloc(MAXLEN * sizeof(char));
  read(fd, dst, MAXLEN);
  printf("%s\n", dst);
  close(fd);
  return 0;
}
int sendResult() {}
