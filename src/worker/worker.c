#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../wrapping/wrapping.h"
#include "worker.h"

#define OK 0
#define WORKING 1
#define NOT_WORKING -1
#define CAST_FAILURE -2
#define READ_DIRECTIVES_FAILURE -3
#define CURSOR_FAILURE -4
#define READ_FAILURE -5
#define WRITE_FAILURE -6

#define WRITE_CHANNEL 1
#define READ_CHANNEL 0
#define MAXLEN 300

/**
 * Inits worker realated operations
 *
 * args:
 *    char *path: path of the file
 *    char *bufferStart: contains buffer start in chars type
 *    char *bufferEnd: contains buffer end in chars type
 *    int *start: contains buffer start in int type
 *    int *end: contains buffer end in int type
 */
int initWork(int *start, int *end);

/**
 * Read all infos from pipe
 *
 * args:
 *    char *path: path of the file
 *    char *bufferStart: contains buffer start in chars type
 *    char *bufferEnd: contains buffer end in chars type
 */
void readDirectives(char *path, char *bufferStart, char *bufferEnd);

/**
 * Runs work
 *
 * args:
 *    const int fd: file descriptor
 *    const int start: start position
 *    const int end: end position
 */
int executeWork(const int fd, const int start, const int end);

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
  int start;
  int end;
  int rc_work = OK;
  int working = WORKING;

  int fd = initWork(&start, &end);
  if (fd < OK) {
    int rc_er = errorHandler(fd, end);
    if (rc_er < OK)
      working = NOT_WORKING;
  }

  while (working == WORKING) {
    rc_work = executeWork(fd, start, end);
    if (rc_work < OK) {
      int rc_er = errorHandler(fd, end);
      if (rc_er < OK)
        working = NOT_WORKING;
    }
    fd = initWork(&start, &end);
    if (fd < OK) {
      int rc_er = errorHandler(fd, end);
      if (rc_er < OK)
        working = NOT_WORKING;
    }
  }

  return working;
}

int initWork(int *start, int *end) {
  int rc_t = 0;
  char path[MAXLEN];
  char bufferStart[MAXLEN];
  char bufferEnd[MAXLEN];

  // TODO fix \0 not read
  readDirectives(path, bufferStart, bufferEnd);

  int rc_sc = sscanf(bufferStart, "%d", start);
  int rc_sc2 = sscanf(bufferEnd, "%d", end);

  if (path[strlen(path) - 1] == '\n') {
    path[strlen(path) - 1] = '\0';
  }

  int fd = openFile(path, O_RDONLY);
  rc_t = fd;

  if (fd == -1)
    rc_t = READ_DIRECTIVES_FAILURE;
  if (rc_sc == 0 || rc_sc2 == 0)
    rc_t = CAST_FAILURE;

  return rc_t;
}

void readDirectives(char *path, char *bufferStart, char *bufferEnd) {
  // TODO choose max length for path
  // TODO ATTENZIONE mettere i byte terminatori nel manager
  char readBuffer[2] = "a";

  int counter = 0;
  do {
    int rc = readChar(READ_CHANNEL, readBuffer);
    path[counter++] = readBuffer[0];
  } while (readBuffer[0] != '\0' && readBuffer[0] != '\n');
  path[counter] = '\0';

  counter = 0;
  do {
    int rc = readChar(READ_CHANNEL, readBuffer);
    bufferStart[counter++] = readBuffer[0];
  } while (readBuffer[0] != '\0' && readBuffer[0] != '\n');
  bufferStart[counter] = '\0';

  counter = 0;
  do {
    int rc = readChar(READ_CHANNEL, readBuffer);
    bufferEnd[counter++] = readBuffer[0];
  } while (readBuffer[0] != '\0' && readBuffer[0] != '\n');
  bufferEnd[counter] = '\0';

  if (path[0] == '\0' || bufferStart[0] == '\0' || bufferEnd[0] == '\0') {
    char *msgErr = (char *)malloc(300);
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

int executeWork(const int fd, const int start, const int end) {
  int rc_t = 0;
  int rc_se = moveCursorFile(fd, start, SEEK_SET);
  if (rc_se == -1)
    rc_t = CURSOR_FAILURE;

  char charRead[2];
  int bytesRead;
  int workAmount = end - start + 1;

  while (workAmount != 0 && rc_t == 0) {
    bytesRead = readChar(fd, charRead);
    if (bytesRead <= 0) {
      rc_t = READ_FAILURE;
    } else {
      // TODO check \0 char after charRead
      int rc_wr = write(WRITE_CHANNEL, charRead, 1);
      if (rc_wr == -1)
        rc_t = WRITE_FAILURE;
      workAmount--;
    }
  }

  if (workAmount == 0 && rc_t != -1) {
    int rc_wr = writeDescriptor(WRITE_CHANNEL, "done");
    if (rc_wr == -1)
      rc_t = WRITE_FAILURE;
  }

  return rc_t;
}

int errorHandler(const int fd, const int end) {
  int rc_t = 0;
  if (fd == -1)
    rc_t = READ_DIRECTIVES_FAILURE;

  int rc_se = moveCursorFile(fd, 0, SEEK_CUR);
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
