#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define READ_PIPE 0
#define WRITE_PIPE 1
#define MAXLEN_PIPE 300

int createPipe(int fd[]) {
  int returnCode = pipe(fd);
  if (returnCode == -1) {
    fprintf(stderr, "%s", "Error: pipe creation gone wrong\n");
  }
  return returnCode;
}

int parentWrite(int fd[], char *msg) {
  if (strlen(msg) + 1 > MAXLEN_PIPE) {
    fprintf(stderr, "%s", "Error: parent message too long\n");
  }
  printf("%d, %d, %s \n", fd[0], fd[1], msg);
  int returnClose = close(fd[READ_PIPE]);
  printf("%d\n", returnClose);
  int returnWrite = write(fd[WRITE_PIPE], msg, strlen(msg) + 1);
  printf("%d\n", returnWrite);
  int returnClose2 = close(fd[WRITE_PIPE]);
  return 0;
}

int readChild(int fd[], char *dst) {
  int returnClose = close(fd[WRITE_PIPE]);
  int bytesRead = read(fd[READ_PIPE], dst, MAXLEN_PIPE);
  if (bytesRead == -1) {
    bytesRead = -1;
    fprintf(stderr, "%s", "Error: reading from pipe in child process\n");
  }
  int returnClose2 = close(fd[READ_PIPE]);
  return bytesRead;
}

int main(int argc, char *argv[]) {
  int fd[2];
  char *msg = "prova";
  char *dst;
  createPipe(fd);
  if (fork() > 0) {
    parentWrite(fd, msg);
  } else {
    readChild(fd, dst);
    printf("dts: %s", dst);
  }
  return 0;
}
