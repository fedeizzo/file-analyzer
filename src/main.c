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
#include "queue/queue.h"
#include "reporter/reporter.h"
#include "worker/worker.h"
#include "wrapping/wrapping.h"

int flag = 0;
void workerDone(int sig) {
  fprintf(stdout, "sono qua detro: %d\n", getpid());
  /* if (done == NULL) */
  /*   printf("cazxo"); */
  /* fprintf(stdout, "mi ha chiamato: %d\n", done->Info.first); */
  flag = 1;
}

int main(int argc, char *argv[]) {
  signal(SIGUSR1, workerDone);
  int pipe1[2], pipe2[2];
  int comm[2];
  createUnidirPipe(comm);
  createBidirPipe(pipe1, pipe2);
  int fd[] = {pipe1[0], pipe1[1], pipe2[0], pipe2[1]};
  if (fork() > 0) {
    int parentFlag = 0;
    parentInitBidPipe(fd);
    parentInitUniPipe(comm);
    parentWriteBidPipe(fd, "./file.txt");
    char dst[300];
    char commMsg[300];
    long long unsigned int table[129];
    for (int i = 0; i < 129; i++) {
      table[i] = 0;
    }
    /* while (strcmp(dst, "done") != 0) { */
    int bytesRead = 0;
    while (bytesRead != 0 || parentFlag == 0) {
      bytesRead = parentReadBidPipe(fd, dst);
      printf("%s", dst);
      table[dst[0]]++;
      if (parentFlag == 0)
        parentReadUniPipe(comm, commMsg);
      if (strcmp(commMsg, "done") == 0)
        parentFlag = 1;
    }
    /* for (int i = 31; i < 129; i++) { */
    /*   printf("%d --- %c → %lli\n", i, (char)i, table[i]); */
    /* } */
    /* fflush(stdout); */
    parentDestroyBidPipe(fd);
  } else {
    childInitBidPipe(fd);
    childInitUniPipe(comm);
    char path[300];
    childReadBidPipe(fd, path);
    int file = openFile(path, O_RDONLY);
    char charRead[300];
    int bytesRead = 0;
    int totale = 0;
    int childFlag = 1;
    /* while (flag == 1 || totale > 100) { */
    childWriteUniPipe(comm, "work");
    while (childFlag == 1) {
      bytesRead = readChar(file, charRead);
      totale += bytesRead;
      /* if (bytesRead == 0 || totale > 100) { */
      if (bytesRead == 0) {
        childWriteUniPipe(comm, "done");
        childFlag = 0;
        /* kill(getppid(), SIGUSR1); */
      } else {
        childWriteBidPipe(fd, charRead);
      }
    }
    childDestroyBidPipe(fd);
  }
  return 0;
}
