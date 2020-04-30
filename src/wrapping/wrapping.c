#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "wrapping.h"

#define READ_UNIDIR 0
#define WRITE_UNIDIR 1

#define READ_PARENT 0
#define WRITE_PARENT 1
#define READ_CHILD 2
#define WRITE_CHILD 3

#define MAXLEN_PIPE 300

#define MAXLEN_ERR 300

void printError(const char *msg) {
  fprintf(stderr, "Error: %s, pid: %d, parentPid: %d\n", msg, getpid(),
          getppid());
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

int readChar(const int fd, char *dst) {
  int bytesRead = read(fd, dst, 1);
  if (bytesRead == -1) {
    printError("during readig char from file");
  }

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

int readDescriptor(const int fd, char dst[]) {
  int bytesRead = read(fd, dst, MAXLEN_PIPE);
  if (bytesRead == -1) {
    bytesRead = -1;
    char *msgErr = (char *)malloc(MAXLEN_ERR);
    sprintf(msgErr, "during reading descriptor: %d", fd);
    printError(msgErr);
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

/* int createUnidirPipe(int fd[]) { */
/*   int returnCode = pipe(fd); */
/*   if (returnCode == -1) { */
/*     fprintf(stderr, "%s", "Error: pipe creation gone wrong\n"); */
/*   } */
/*   return returnCode; */
/* } */

/* int parentInitUniPipe(const int fd[]) { */
/*   int rc_t = 0; */
/*   int rc_cl = closeDescriptor(fd[READ_PIPE]); */

/*   if (rc_cl == -1) */
/*     rc_t = -1; */

/*   return rc_t; */
/* } */

/* int childInitUniPipe(const int fd[]) { */
/*   int rc_t = 0; */
/*   int rc_cl = closeDescriptor(fd[WRITE_PIPE]); */

/*   if (rc_cl == -1) */
/*     rc_t = -1; */

/*   return rc_t; */
/* } */

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
  int bytesRead = readDescriptor(fd[READ_PARENT], dst);

  return bytesRead;
}

int parentReadBidPipe(const int fd[], char *dst) {
  int bytesRead = readDescriptor(fd[READ_CHILD], dst);

  return bytesRead;
}
