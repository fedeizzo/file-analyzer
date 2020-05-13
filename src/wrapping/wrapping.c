#include "wrapping.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define READ_UNIDIR 0
#define WRITE_UNIDIR 1

#define READ_PARENT 0
#define WRITE_PARENT 1
#define READ_CHILD 2
#define WRITE_CHILD 3

#define MAXLEN_PIPE 300

#define MAXLEN_ERR 300

void printError(const char *msg) {
  fprintf(stderr, "ERROR: %s, pid: %d, parentPid: %d\n", msg, getpid(),
          getppid());
}

void printInfo(const char *msg) {
  fprintf(stderr, "INFO: %s (no fatal), pid: %d, parentPid: %d\n", msg,
          getpid(), getppid());
}

int checkAllocationError(void *ptr) {
  int rc_t = 0;
  if (ptr == NULL) {
    printError("error during malloc allocation");
    rc_t = -1;
  }

  return rc_t;
}

int openFile(const char *path, const int mode) {
  int code = open(path, mode);
  if (code == -1) {
    char *msgErr = (char *)malloc(MAXLEN_ERR);
    sprintf(msgErr, "during opening file: %s", path);
    printError(msgErr);
  }
  return code;
}

int moveCursorFile(const int fd, const int position, const int absPosition) {
  int rc_t = 0;
  int rc_se = lseek(fd, position, absPosition);
  if (rc_se == -1) {
    char *msgErr = (char *)malloc(MAXLEN_ERR);
    sprintf(msgErr, "during move cursor in descriptor: %d", fd);
    printError(msgErr);
    free(msgErr);
  }
  rc_t = rc_se;

  return rc_t;
}

int readChar(const int fd, char *dst) {
  int bytesRead = read(fd, dst, 1);
  if (bytesRead == -1) {
    /* printError("during readig char from file"); */
  }
  // TODO fix this strange thing
  /* dst[bytesRead] = '\0'; */

  return bytesRead;
}

int closeDescriptor(const int fd) {
  int code = close(fd);
  if (code == -1) {
    char *msgErr = (char *)malloc(MAXLEN_ERR);
    sprintf(msgErr, "during closing descriptor: %d", fd);
    printError(msgErr);
  }
  return code;
}

int createDup(const int writer, const int overwritten) {
  int rc_du = dup2(writer, overwritten);

  if (rc_du == -1) {
    char *msgErr = (char *)malloc(MAXLEN_ERR);
    sprintf(msgErr, "during dup creation with descriptors: %d, %d", writer,
            overwritten);
    printError(msgErr);
    free(msgErr);
  }

  return rc_du;
}

int readDescriptor(const int fd, char dst[], const int len) {
  int bytesRead = read(fd, dst, len);
  /* printf("\tho letto: %d\n", bytesRead); */
  dst[bytesRead] = '\0';
  // TODO free msg in all functions of this file
  if (bytesRead == -1) {
    bytesRead = -1;
    char *msgErr = (char *)malloc(MAXLEN_ERR);
    sprintf(msgErr, "during reading descriptor: %d", fd);
    printError(msgErr);
    free(msgErr);
  }
  return bytesRead;
}

int writeDescriptor(const int fd, const char msg[]) {
  int code = write(fd, msg, strlen(msg) + 1);
  if (code == -1) {
    code = -1;
    char *msgErr = (char *)malloc(MAXLEN_ERR);
    sprintf(msgErr, "during writing descriptor: %d", fd);
    printError(msgErr);
  }
  return code;
}

int createUnidirPipe(int fd[]) {
  int returnCode = pipe(fd);
  if (returnCode == -1) {
    fprintf(stderr, "%s", "Error: pipe creation gone wrong\n");
  }
  return returnCode;
}

int parentInitUniPipe(const int fd[]) {
  int rc_t = 0;
  int rc_cl = closeDescriptor(fd[WRITE_UNIDIR]);

  if (rc_cl == -1)
    rc_t = -1;

  return rc_t;
}

int childInitUniPipe(const int fd[]) {
  int rc_t = 0;
  int rc_cl = closeDescriptor(fd[READ_UNIDIR]);

  if (rc_cl == -1)
    rc_t = -1;

  return rc_t;
}

int childWriteUniPipe(const int fd[], const char *msg) {
  int rc_tl = 0;
  if (strlen(msg) + 1 > MAXLEN_PIPE) {
    printError("parent message too long");
    rc_tl = -1;
  }
  int rc_wr = writeDescriptor(fd[WRITE_UNIDIR], msg);

  if (rc_wr == -1)
    rc_tl = -1;
  return rc_tl;
}

int parentReadUniPipe(const int fd[], char *dst) {
  int bytesRead = readDescriptor(fd[READ_UNIDIR], dst, 5);
  dst[bytesRead] = '\0';

  return bytesRead;
}

int createBidirPipe(int pipe1[], int pipe2[]) {
  int code = pipe(pipe1);
  int code2 = pipe(pipe2);
  if (code == -1 || code2 == -1) {
    fprintf(stderr, "%s", "Error: pipe creation gone wrong\n");
  }
  return code;
}

int parentInitBidPipe(const int fd[]) {
  int rc_t = 0;
  int rc_cl = closeDescriptor(fd[READ_PARENT]);
  int rc_cl2 = closeDescriptor(fd[WRITE_CHILD]);

  if (rc_cl == -1 || rc_cl2 == -1)
    rc_t = -1;

  return rc_t;
}

int childInitBidPipe(const int fd[]) {
  int rc_t = 0;
  int rc_cl = closeDescriptor(fd[READ_CHILD]);
  int rc_cl2 = closeDescriptor(fd[WRITE_PARENT]);

  if (rc_cl == -1 || rc_cl2 == -1)
    rc_t = -1;

  return rc_t;
}

int parentDestroyBidPipe(const int fd[]) {
  int rc_t = 0;
  int rc_cl = closeDescriptor(fd[READ_CHILD]);
  int rc_cl2 = closeDescriptor(fd[WRITE_PARENT]);

  if (rc_cl == -1 || rc_cl2 == -1)
    rc_t = -1;

  return rc_t;
}

int childDestroyBidPipe(const int fd[]) {
  int rc_t = 0;
  int rc_cl = closeDescriptor(fd[READ_PARENT]);
  int rc_cl2 = closeDescriptor(fd[WRITE_CHILD]);

  if (rc_cl == -1 || rc_cl2 == -1)
    rc_t = -1;

  return rc_t;
}

int childWriteBidPipe(const int fd[], const char *msg) {
  int rc_tl = 0;
  if (strlen(msg) + 1 > MAXLEN_PIPE) {
    printError("parent message too long");
    rc_tl = -1;
  }
  int rc_wr = writeDescriptor(fd[WRITE_CHILD], msg);
  /* printf("scrivo in: %d", fd[WRITE_CHILD]); */

  if (rc_wr == -1)
    rc_tl = -1;
  return rc_tl;
}

int parentWriteBidPipe(const int fd[], const char *msg) {
  int rc_tl = 0;
  if (strlen(msg) + 1 > MAXLEN_PIPE) {
    printError("parent message too long");
    rc_tl = -1;
  }
  int rc_wr = writeDescriptor(fd[WRITE_PARENT], msg);

  if (rc_wr == -1)
    rc_tl = -1;
  return rc_tl;
}

int childReadBidPipe(const int fd[], char *dst) {
  int bytesRead = readDescriptor(fd[READ_PARENT], dst, 300);

  return bytesRead;
}

int parentReadBidPipe(const int fd[], char *dst) {
  int bytesRead = readDescriptor(fd[READ_CHILD], dst, 2);
  dst[bytesRead] = '\0';
  /* printf("leggo da: %d\n", fd[READ_CHILD]); */

  return bytesRead;
}
