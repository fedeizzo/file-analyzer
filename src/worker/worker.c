#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../wrapping/wrapping.h"
#include "worker.h"

#define WRITE_CHANNEL 0
#define READ_CHANNEL 1
#define MAXLEN 300

int workerExec(const int readPipe[], const int writePipe[],
               const char *exeFile) {
  int rc_t = 0;
  // TODO change close to close descriptor
  int rc_cl = close(writePipe[READ]);
  int rc_cl2 = close(readPipe[WRITE]);
  // TODO create wrapping function for dup2
  int rc_du = dup2(writePipe[WRITE], 0);
  int rc_du2 = dup2(readPipe[READ], 1);
  int rc_cl3 = close(writePipe[WRITE]);
  int rc_cl4 = close(readPipe[READ]);
  if (rc_cl == -1 || rc_cl2 == -1 || rc_cl3 == -1 || rc_cl4 == -1 ||
      rc_du == -1 || rc_du2 == -1) {
    rc_t = -1;
  }

  // TODO nel figlio usare descrittore 0 per scriveere e 1 per leggere
  execlp(exeFile, exeFile, NULL);

  return rc_t;
}

// TODO is the same of bidirectional pipe. ???
int parentInitExecPipe(const int readPipe[], const int writePipe[]) {
  int rc_t = 0;
  // TODO change to closeDescriptor
  int rc_cl = close(readPipe[WRITE]);
  int rc_cl2 = close(writePipe[READ]);

  if (rc_cl == -1 || rc_cl2 == -1) {
    rc_t = -1;
  }
  return rc_t;
}

// TODO is the same of bidirectional pipe. ???
int parentDestroyExecPipe(int readPipe[], int writePipe[]) {
  int rc_t = 0;

  // TODO change to closeDescriptor
  int rc_cl = close(readPipe[READ]);
  int rc_cl2 = close(writePipe[WRITE]);
  if (rc_cl == -1 || rc_cl2 == -1) {
    rc_t = -1;
  }
  return rc_t;
}

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
  int rc_work = 0;

  int fd = initWork(&start, &end);
  if (fd == -1) {
    int rc_er = errorHandler(fd, end);
    if (rc_er == -1)
      return -1;
  }

  while (1) {
    rc_work = executeWork(fd, start, end);
    if (rc_work == -1) {
      int rc_er = errorHandler(fd, end);
      if (rc_er == -1)
        return -1;
    }
    fd = initWork(&start, &end);
    if (fd == -1) {
      int rc_er = errorHandler(fd, end);
      if (rc_er == -1)
        return -1;
    }
  }

  return 0;
}

int initWork(int *start, int *end) {
  char *path;
  char *bufferStart;
  char *bufferEnd;
  readDirectives(path, bufferStart, bufferEnd);
  // TODO fare verifica sui cast
  *start = (int)bufferStart;
  *end = (int)bufferEnd;

  int fd = openFile(path, O_RDONLY);

  return fd;
}

void readDirectives(char *path, char *bufferStart, char *bufferEnd) {
  // TODO choose max length for path
  // TODO ATTENZIONE mettere i byte terminatori nel manager
  int pathRead = readDescriptor(READ_CHANNEL, path, MAXLEN);
  int buffStrRead = readDescriptor(READ_CHANNEL, bufferStart, MAXLEN);
  int buffEndRead = readDescriptor(READ_CHANNEL, bufferEnd, MAXLEN);

  if (buffEndRead == -1 || buffStrRead == -1 || pathRead == -1) {
    // TODO fare il free
    char *msgErr = (char *)malloc(300);
    sprintf(msgErr, "inisde worker with pid: %d", getpid());
    printError(msgErr);
  }
}

int executeWork(const int fd, const int start, const int end) {
  int rc_t = 0;
  int rc_se = moveCursorFile(fd, start);
  if (rc_se == -1)
    rc_t = rc_se;

  char charRead[2];
  int bytesRead;
  int workAmount = end - start + 1;

  while (workAmount != 0 && rc_t == 0) {
    bytesRead = readChar(fd, charRead);
    if (bytesRead == -1) {
      rc_t = -1;
    } else {
      int rc_wr = writeDescriptor(WRITE_CHANNEL, charRead);
      if (rc_wr == -1)
        rc_t = -1;
      workAmount--;
    }
  }

  if (workAmount == 0 && rc_t != -1) {
    int rc_wr = writeDescriptor(WRITE_CHANNEL, "done");
    if (rc_wr == -1)
      rc_t = -1;
  }

  return rc_t;
}

int errorHandler(const int fd, const int end) {
  int rc_t = 0;
  if (fd == -1)
    rc_t = -1;

  // TODO fix LSEEK_SET
  int rc_se = moveCursorFile(fd, 0);
  if (rc_se == -1)
    rc_t = rc_se;

  int workAmount = end - rc_se + 1;

  while (workAmount != 0 && rc_t == 0) {
    int rc_wr = writeDescriptor(WRITE_CHANNEL, "a");
    if (rc_wr == -1)
      rc_t = -1;
    else
      workAmount--;
  }

  if (workAmount == 0 && rc_t != -1) {
    int rc_wr = writeDescriptor(WRITE_CHANNEL, "erro");
    if (rc_wr == -1)
      rc_t = -1;
  }

  return rc_t;
}
