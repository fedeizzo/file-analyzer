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

#include "analyzer/analyzer.h"
#include "manager/manager.h"
#include "reporter/reporter.h"
#include "worker/worker.h"
#include "wrapping/wrapping.h"

int main(int argc, char *argv[]) {
  int pipe1[2], pipe2[2];
  createBidirPipe(pipe1, pipe2);
  int fd[] = {pipe1[0], pipe1[1], pipe2[0], pipe2[1]};
  if (fork() > 0) {
    parentInitBidPipe(fd);
    parentWriteBidPipe(fd, "./file.txt");
    char dst[300];
    while (strcmp(dst, "done") != 0) {
      parentReadBidPipe(fd, dst);
      printf("%s", dst);
    }
    printf("%s", dst);
    parentDestroyBidPipe(fd);
  } else {
    childInitBidPipe(fd);
    char path[300];
    childReadBidPipe(fd, path);
    int file = openFile(path, O_RDONLY);
    char charRead;
    int bytesRead = 0;
    int flag = 1;
    while (flag == 1) {
      bytesRead = readChar(file, &charRead);
      if (bytesRead == 0) {
        childWriteBidPipe(fd, "done");
        flag = 0;
      } else
        childWriteBidPipe(fd, &charRead);
    }
    childDestroyBidPipe(fd);
  }
  return 0;
}
