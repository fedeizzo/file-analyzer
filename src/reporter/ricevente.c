#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../list/list.h"
#include "../wrapping/wrapping.h"

char *readString(int fd, char *dst) {
  int index = 0;
  int byteRead = -1;
  char charRead = 'a';

  while (byteRead != 0 && charRead != '\0') {
    byteRead = readChar(fd, &charRead);
    /* if (charRead != 'a') */
    /* printf("%d %c %d\n", charRead, charRead, byteRead); */
    if (byteRead != 0) {
      dst[index++] = charRead;
    }
  }
  /* dst[index] = '\0'; */

  return dst;
}

int main() {
  char *readFifo = "/tmp/reporterToAnalyzer";
  char *writeFifo = "/tmp/analyzerToReporter";
  /* remove(writeFifo); */
  int rc_fi = mkfifo(readFifo, 0666);
  int rc_fi2 = mkfifo(writeFifo, 0666);
  int fifoDescriptor;
  List dire = newList();

  while (1) {
    int fd = open(readFifo, O_RDONLY);
    while (1) {
      char *dst = malloc(PATH_MAX * sizeof(char));
      dst[0] = '\0';
      dst = readString(fd, dst);
      /* printf("parola di controllo: %s\n", dst); */
      if (strcmp(dst, "dire") == 0) {
        char *msg = front(dire);
        /* printf("size %d\n", dire->size); */
        printf("path: %s\n", msg);
        pop(dire);
        free(msg);
        msg = front(dire);
        /* printf("size %d\n", dire->size); */
        printf("manager: %s\n", msg);
        pop(dire);
        free(msg);
        msg = front(dire);
        /* printf("size %d\n", dire->size); */
        printf("worker: %s\n", msg);
        pop(dire);
        free(msg);
        break;
      } else if (strcmp(dst, "requ") == 0) {
        while (dire->size != 0) {
          char *msg = front(dire);
          printf("file: %s\n", msg);
          pop(dire);
          free(msg);
        }
        break;
      } else if (strcmp(dst, "tree") == 0) {
        char *msg = front(dire);
        printf("voglio i figli di : %s\n", msg);
        pop(dire);
        free(msg);
        break;
      } else if (strcmp(dst, "") != 0) {
        enqueue(dire, dst);
        /* printf("ho encodato %s, newSize %d\n", dst, dire->size); */
      } else {
        /* printf("ho fatto la free\n"); */
        free(dst);
      }
      usleep(1);
    }
    close(fd);
    printf("chiudo\n");
  }

  /* fd = open(fifo, O_WRONLY); */
  /* char *msg = "cioa bello"; */
  /* write(fd, msg, strlen(msg) + 1); */
  /* close(fd); */
}
