#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../list/list.h"
#include "../wrapping/wrapping.h"
#include "./reporter.h"

#define OK 0
#define MAXLEN 4096

#define CAST_FAILURE -2
#define SEND_FAILURE -3

#define WRITE_CHANNEL 1
#define READ_CHANNEL 0

UserInput newUserInput() {
  UserInput ui = malloc(sizeof(struct UserInputStr));
  int rc_al = checkAllocationError(ui);
  int rc_al2 = OK;
  int rc_al3 = OK;
  int rc_al4 = OK;

  if (rc_al == OK) {
    ui->paths = newList();
    ui->results = newList();
    ui->tree = malloc(MAXLEN * sizeof(char));
    if (ui->paths == NULL)
      rc_al2 = -1;
    if (ui->results == NULL)
      rc_al3 = -1;
    rc_al4 = checkAllocationError(ui->paths);
    ui->managers = 3;
    ui->workers = 4;
  }

  if (rc_al < OK || rc_al2 < OK || rc_al3 < OK || rc_al4 < OK)
    ui = NULL;

  return ui;
}

void destroyUserInput(UserInput userInput) {
  destroyList(userInput->paths, free);
  destroyList(userInput->results, free);
  free(userInput->tree);
}

void *userInputLoop(void *ptr);
int readDirectives(List paths, int *numManager, int *numWorker);

void *writeFifoLoop(void *ptr);
int sendDirectives(int fd, char *path, int *numManager, int *numWorker);
int readResult(List pathResults);
int sendResult(int fd, List pathResults);
int readTree(char *path);
int sendTree(int fd, char *treePath);

int main() {
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

  destroyUserInput(userInput);
  return rc_t;
}

void *userInputLoop(void *ptr) {
  userInput_t *input = (userInput_t *)ptr;

  int rc_t = OK;
  while (rc_t == OK) {
    char *dst = malloc(MAXLEN * sizeof(char));
    int bytesRead = read(0, dst, 5);
    if (bytesRead > 0) {
      if (strncmp(dst, "dire", 4) == 0) {
        pthread_mutex_lock(&(input->mutex));
        rc_t = readDirectives(input->userInput->paths,
                              &(input->userInput->managers),
                              &(input->userInput->workers));
        pthread_mutex_unlock(&(input->mutex));
      } else if (strncmp(dst, "requ", 4) == 0) {
        pthread_mutex_lock(&(input->mutex));
        readResult(input->userInput->results);
        pthread_mutex_unlock(&(input->mutex));
      } else if (strncmp(dst, "tree", 4) == 0) {
        printf("entro nel tree\n");
        pthread_mutex_lock(&(input->mutex));
        readTree(input->userInput->tree);
        printf("esco dal tree: %s\n", input->userInput->tree);
        pthread_mutex_unlock(&(input->mutex));
      }
    }
    usleep(50000);
  }
  printf("osno morto\n");
}

int isAnalyzerAlive() {
  int rc_t = -1;
  int pid = fork();

  if (pid == 0) {
    execlp("ps -a | grep 'ricevente'", "ps -a | grep 'ricevente'", NULL);
  } else if (pid > 0) {
    waitpid(pid, &rc_t, WNOHANG);

    WEXITSTATUS(rc_t);
  }

  return rc_t;
}

void *writeFifoLoop(void *ptr) {
  userInput_t *input = (userInput_t *)ptr;

  int rc_t = OK;
  int fd;
  char *fifoPath = malloc(MAXLEN * sizeof(char));
  pthread_mutex_lock(&(input->mutex));
  strcpy(fifoPath, input->writeFifo);
  pthread_mutex_unlock(&(input->mutex));

  while (rc_t == OK) {
    pthread_mutex_lock(&(input->mutex));
    if (input->userInput->paths->size > 0) {
      char *path = front(input->userInput->paths);
      if (path != NULL) {
        fd = open(fifoPath, O_WRONLY);
        if (fd > 0) {
          int rc_po = pop(input->userInput->paths);
          rc_t = sendDirectives(fd, path, &(input->userInput->managers),
                                &(input->userInput->workers));
        }
        close(fd);
        free(path);
      }
    }
    if (input->userInput->results->size > 0) {
      fd = open(fifoPath, O_WRONLY);
      if (fd > 0) {
        printf("sto per entrare\n");
        rc_t = sendResult(fd, input->userInput->results);
        printf("entrato\n");
      }
      close(fd);
    }
    if (input->userInput->tree[0] != '\0') {
      fd = open(fifoPath, O_WRONLY);
      if (fd > 0) {
        printf("sto per entrare\n");
        rc_t = sendTree(fd, input->userInput->tree);
        printf("entrato\n");
      }
      close(fd);
    }
    pthread_mutex_unlock(&(input->mutex));
    usleep(500);
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
      sprintf(msgErr, "inisde reporter with pid: %d", getpid());
      printError(msgErr);
      free(msgErr);
    }
  } else if (rc_t == OK) {
    enqueue(paths, newPath);
    *numManager = numberManager;
    *numWorker = numberWorker;
  }

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
    /* int rc_wr = writeDescriptor(fd, "dire"); */
    int rc_wr = OK;
    int rc_wr1 = writeDescriptor(fd, path);
    int rc_wr2 = writeDescriptor(fd, nManager);
    int rc_wr3 = writeDescriptor(fd, nWorker);
    int rc_wr4 = writeDescriptor(fd, "dire");

    if (rc_wr < OK || rc_wr2 < OK || rc_wr3 < OK || rc_wr4 < OK)
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
  else {
    /* closeDescriptor(fd); */
  }

  return rc_t;
}

int readResult(List pathResults) {
  int rc_t = OK;
  int endFlag = 1;

  while (endFlag && rc_t == OK) {
    char *path = malloc(MAXLEN * sizeof(char));
    int index = 0;
    char charRead = 'a';
    int bytesRead = -1;
    rc_t = checkAllocationError(path);
    while (bytesRead != 0 && charRead != '\n') {
      bytesRead = readChar(0, &charRead);
      if (bytesRead > 0)
        path[index++] = charRead;
    }
    path[--index] = '\0';

    if (strncmp(path, "requ", 4) == 0) {
      endFlag = 0;
    } else if (path[0] != '\0' && rc_t == OK)
      rc_t = enqueue(pathResults, path);
  }

  return rc_t;
}

int sendResult(int fd, List pathResults) {
  int rc_t = OK;
  printf("enotr qui\n");

  while (pathResults->size > 0 && rc_t == OK) {
    char *path = front(pathResults);
    if (path != NULL) {
      printf("invio %s\n", path);
      rc_t = pop(pathResults);
      if (rc_t == OK) {
        int rc_wr = writeDescriptor(fd, path);
        if (rc_wr < OK)
          rc_t = -1;
      }
    }
  }

  writeDescriptor(fd, "requ");

  return rc_t;
}

int readTree(char *path) {
  int rc_t = OK;

  int index = 0;
  char charRead = 'a';
  int bytesRead = -1;
  while (bytesRead != 0 && charRead != '\n') {
    bytesRead = readChar(0, &charRead);
    if (bytesRead > 0)
      path[index++] = charRead;
  }
  path[--index] = '\0';

  return rc_t;
}

int sendTree(int fd, char *treePath) {
  int rc_t = OK;

  int rc_wr = writeDescriptor(fd, treePath);
  if (rc_wr < OK)
    rc_t = -1;

  writeDescriptor(fd, "tree");

  treePath[0] = '\0';

  return rc_t;
}
