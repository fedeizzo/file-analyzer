#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "wrapping.h"

#define READ_PIPE 0
#define WRITE_PIPE 1
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
  }
  return bytesRead;
}

int writeDescriptor(const int fd, char msg[]) {
  int code = write(fd, msg, strlen(msg) + 1);
  if (code == -1) {
    code = -1;
    char *msgErr = (char *)malloc(MAXLEN_ERR);
    sprintf(msgErr, "during writing descriptor: %d", fd);
  }
  return code;
}

int createPipe(int fd[]) {
  int returnCode = pipe(fd);
  if (returnCode == -1) {
    fprintf(stderr, "%s", "Error: pipe creation gone wrong\n");
  }
  return returnCode;
}

int parentWrite(const int fd[], const char *msg) {
  int rc_t = 0;
  int rc_tl = 0;
  if (strlen(msg) + 1 > MAXLEN_PIPE) {
    printError("parent message too long");
    rc_tl = -1;
  }
  int rc_cl = closeDescriptor(fd[READ_PIPE]);
  int rc_wr = write(fd[WRITE_PIPE], msg, strlen(msg) + 1);
  int rc_cl2 = closeDescriptor(fd[WRITE_PIPE]);

  if (rc_cl == -1 || rc_cl2 == -1 || rc_wr == -1 || rc_tl == -1)
    rc_t = -1;

  return rc_t;
}

int readChild(const int fd[], char *dst) {
  int rc_t = 0;
  int rc_cl = closeDescriptor(fd[WRITE_PIPE]);
  int bytesRead = readDescriptor(fd[READ_PIPE], dst);
  int rc_cl2 = closeDescriptor(fd[READ_PIPE]);
  if (rc_cl == -1 || rc_cl2 == -1 || bytesRead == -1)
    rc_t = -1;
  else
    rc_t = bytesRead;

  return rc_t;
}
