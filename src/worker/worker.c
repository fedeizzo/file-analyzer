#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../config/config.h"
#include "../wrapping/wrapping.h"
#include "worker.h"
#include <limits.h>

/**
 * Inits worker realated operations
 *
 * args:
 *    char *path: path of the file
 *    char *bufferStart: contains buffer start in chars type
 *    char *bufferEnd: contains buffer end in chars type
 *    int *start: contains buffer start in int type
 *    int *end: contains buffer end in int type
 *    int *stopFlag: flags that indicates if the worker must stop
 *
 * returns:
 *    positive (file descriptor) in case of success, otherwise negative
 */
int initWork(unsigned long long *start, unsigned long long *end, int *stopFlag);

/**
 * Read all infos from pipe
 *
 * args:
 *    char *path: path of the file
 *    char *bufferStart: contains buffer start in chars type
 *    char *bufferEnd: contains buffer end in chars type
 *    int *stopFlag: flags that indicates if the worker must stop
 *
 * returns:
 *    0 in case of success, otherwise negative
 */
void readDirectives(char *path, char *bufferStart, char *bufferEnd,
                    int *stopFlag);

/**
 * Runs work
 *
 * args:
 *    const int fd: file descriptor
 *    const int start: start position
 *    const int end: end position
 *
 * returns:
 *    0 in case of success, otherwise negative
 */
int executeWork(const int fd, const unsigned long long start,
                const unsigned long long end);

/**
 * Sends an acknowledgment to the father
 *
 * returns:
 *    0 in case of success, otherwise negative
 */
int sendAcknowledgment();

/**
 * Communicates to manager that happen something wrong.
 * If this function fails worker terminates its execution
 * returning -1
 *
 * arga:
 *    const int fd: file descriptor
 *    const int end: file buffer end
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int errorHandler(const int fd, const int end);

int main(int argc, char *argv[]) {
  /*int non_lo_so = openFile("err.txt", O_WRONLY);
  dup2(non_lo_so, 2);
  printError("Quello che vuoi");*/

  unsigned long long start;
  unsigned long long end;
  int rc_work = SUCCESS;
  int working = WORKING;
  int stopFlag = 0;
  int rc_ack = SUCCESS;

  int fd = initWork(&start, &end, &stopFlag);
  if (stopFlag == 1) {
    rc_ack = sendAcknowledgment();
    stopFlag = 0;
    if (rc_ack < SUCCESS)
      working = NOT_WORKING;
    else
      fd = initWork(&start, &end, &stopFlag);
  }

  if (fd < SUCCESS) {
    int rc_er = errorHandler(fd, end);
    if (rc_er < SUCCESS)
      working = NOT_WORKING;
  }

  while (working == WORKING) {
    rc_work = executeWork(fd, start, end);
    if (rc_work < SUCCESS) {
      int rc_er = errorHandler(fd, end);
      if (rc_er < SUCCESS)
        working = NOT_WORKING;
    }
    fd = initWork(&start, &end, &stopFlag);
    if (stopFlag == 1) {
      rc_ack = sendAcknowledgment();
      stopFlag = 0;
      if (rc_ack < SUCCESS)
        working = NOT_WORKING;
      else
        fd = initWork(&start, &end, &stopFlag);
    }

    if (fd < SUCCESS) {
      int rc_er = errorHandler(fd, end);
      if (rc_er < SUCCESS)
        working = NOT_WORKING;
    }
  }

  return working;
}

int initWork(unsigned long long *start, unsigned long long *end,
             int *stopFlag) {
  int rc_t = 0;
  char path[PATH_MAX];
  char bufferStart[PATH_MAX];
  char bufferEnd[PATH_MAX];

  // TODO fix \0 not read
  readDirectives(path, bufferStart, bufferEnd, stopFlag);

  if (*stopFlag == 0) {
    int rc_sc = sscanf(bufferStart, "%llu", start);
    int rc_sc2 = sscanf(bufferEnd, "%llu", end);

    if (path[strlen(path) - 1] == '\n') {
      path[strlen(path) - 1] = '\0';
    }

    int fd = openFile(path, O_RDONLY);
    rc_t = fd;

    if (fd == -1)
      rc_t = READ_DIRECTIVES_FAILURE;
    if (rc_sc == 0 || rc_sc2 == 0)
      rc_t = CAST_FAILURE;
  }

  return rc_t;
}

void readDirectives(char *path, char *bufferStart, char *bufferEnd,
                    int *stopFlag) {
  // TODO choose max length for path
  // TODO ATTENZIONE mettere i byte terminatori nel manager
  char readBuffer[2] = "a";
  *stopFlag = 0;

  int counter = 0;
  do {
    int rc = readChar(READ_CHANNEL, readBuffer);
    path[counter++] = readBuffer[0];
  } while (readBuffer[0] != '\0' && readBuffer[0] != '\n');
  path[counter] = '\0';

  if (strncmp(path, "stop", 4) == 0)
    *stopFlag = 1;

  counter = 0;
  do {
    int rc = readChar(READ_CHANNEL, readBuffer);
    bufferStart[counter++] = readBuffer[0];
  } while (readBuffer[0] != '\0' && readBuffer[0] != '\n');
  bufferStart[counter] = '\0';

  if (strncmp(bufferStart, "stop", 4) == 0 && *stopFlag == 1)
    *stopFlag = 1;
  else
    *stopFlag = 0;

  counter = 0;
  do {
    int rc = readChar(READ_CHANNEL, readBuffer);
    bufferEnd[counter++] = readBuffer[0];
  } while (readBuffer[0] != '\0' && readBuffer[0] != '\n');
  bufferEnd[counter] = '\0';

  if (strncmp(bufferEnd, "stop", 4) == 0 && *stopFlag == 1)
    *stopFlag = 1;
  else
    *stopFlag = 0;

  if (path[0] == '\0' || bufferStart[0] == '\0' || bufferEnd[0] == '\0') {
    char *msgErr = (char *)malloc(sizeof(char) * 300);
    int rc_ca = checkAllocationError(msgErr);
    if (rc_ca < 0) {
      printError("I can't allocate memory");
    } else {
      sprintf(msgErr, "inisde worker with pid: %d", getpid());
      printError(msgErr);
      free(msgErr);
    }
  }
}

int executeWork(const int fd, const unsigned long long start,
                const unsigned long long end) {
  int rc_t = 0;
  // TODO... edited here
  // int rc_se = moveCursorFile(fd, start, SEEK_SET);
  // printf("start: %d, end: %d\n", start, end);
  int rc_se = lseek(fd, start, SEEK_SET);
  if (rc_se == -1)
    rc_t = CURSOR_FAILURE;

  unsigned long long bytesRead;
  unsigned long long workAmount = end - start + 1;
  char *charsRead = malloc((workAmount + 2) * sizeof(char));
  int rc_al = checkAllocationError(charsRead);
  int lectures = 1;
  if (workAmount > 1500000000) {
    lectures = (int)(workAmount / 1500000000);
    workAmount = 1500000000;
    if ((workAmount % 1500000000) > 0)
      lectures++;
  }

  if (rc_al == SUCCESS) {
    while (lectures != 0) {
      bytesRead = readDescriptor(fd, charsRead, workAmount);
      int i;
      if (bytesRead > 0)
        for (i = 0; i < workAmount; i++)
          if ((charsRead[i] < 32 || charsRead[i] > 127) && charsRead[i] != '\n')
            charsRead[i] = -15;

      if (lectures == 1)
        charsRead[bytesRead] = '\0';
      if (bytesRead > 0) {
        // fprintf(stderr,"Charsread: %s\n", charsRead);
        int rc_wr = write(WRITE_CHANNEL, charsRead, bytesRead);
        if (rc_wr == -1)
          rc_t = WRITE_FAILURE;
      } else {
        rc_t = READ_FAILURE;
      }
      lectures--;
    }

    // TODO fix this check
    if (rc_t != -1) {
      int rc_wr = writeDescriptor(WRITE_CHANNEL, "done");
      if (rc_wr == -1)
        rc_t = WRITE_FAILURE;
    }
    free(charsRead);
  } else {
    rc_t = -1; // TODO fix
  }
  closeDescriptor(fd);
  return rc_t;
}

int sendAcknowledgment() {
  int rc_t = SUCCESS;
  rc_t = writeDescriptor(WRITE_CHANNEL, "ackn");
  return rc_t;
}

int errorHandler(const int fd, const int end) {
  int rc_t = 0;
  if (fd == -1)
    rc_t = READ_DIRECTIVES_FAILURE;
  // TODO... edited here
  // int rc_se = moveCursorFile(fd, 0, SEEK_CUR);
  int rc_se = lseek(fd, 0, SEEK_CUR);
  if (rc_se == -1)
    rc_t = CURSOR_FAILURE;

  int workAmount = end - rc_se + 1;

  while (workAmount != 0 && rc_t == 0) {
    int rc_wr = writeDescriptor(WRITE_CHANNEL, "a");
    if (rc_wr == -1)
      rc_t = WRITE_FAILURE;
    else
      workAmount--;
  }

  if (workAmount == 0 && rc_t != -1) {
    int rc_wr = writeDescriptor(WRITE_CHANNEL, "erro");
    if (rc_wr == -1)
      rc_t = WRITE_FAILURE;
  }

  return rc_t;
}
