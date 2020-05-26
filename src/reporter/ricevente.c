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

int readString(int fd, char *dst) {
  int index = 0;
  int byteRead = -1;
  char charRead = 'a';

  int lettoTotale = 0;
  while (byteRead != 0 && charRead != '\0') {
    byteRead = readChar(fd, &charRead);
    /* if (charRead != 'a') */
    /* printf("%d %c %d\n", charRead, charRead, byteRead); */
    if (byteRead > 0) {
      dst[index++] = charRead;
      lettoTotale += lettoTotale;
    } else {
      printf("per simoe ho letto %d\n", byteRead);
    }
  }
  return lettoTotale;
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
    printf("sto peer fare la open\n");
    int fd = open(readFifo, O_RDONLY);
    if (fd == -1)
      mkfifo(readFifo, 0666);
    while (1) {
      char *dst = malloc(PATH_MAX * sizeof(char));
      dst[0] = '\0';
      int hoLettoBoiaDio = readString(fd, dst);
      /* int hoLettoBoiaDio = read(fd, dst, 4096); */
      if (hoLettoBoiaDio > 0) {
        int i = 0;
        for (i = 0; i < hoLettoBoiaDio; i++) {
          printf("%c", dst[i]);
        }
        fflush(stdout);
      }
      if (strcmp(dst, "dire") == 0) {
        char *msg = front(dire);
        /* printf("size %d\n", dire->size); */
        /* printf("path: %s\n", msg); */
        pop(dire);
        free(msg);
        msg = front(dire);
        /* printf("size %d\n", dire->size); */
        /* printf("manager: %s\n", msg); */
        pop(dire);
        free(msg);
        msg = front(dire);
        /* printf("size %d\n", dire->size); */
        /* printf("worker: %s\n", msg); */
        pop(dire);
        free(msg);
        /* break; */
      } else if (strcmp(dst, "requ") == 0) {
        while (dire->size != 0) {
          char *msg = front(dire);
          /* printf("file: %s\n", msg); */
          pop(dire);
          free(msg);
        }
        /* break; */
      } else if (strcmp(dst, "tree") == 0) {
        char *msg = front(dire);
        printf("voglio i figli di : %s\n", msg);
        pop(dire);
        free(msg);
        /* break; */
      } else if (strcmp(dst, "") != 0) {
        enqueue(dire, dst);
        printf("ho encodato %s, newSize %d\n", dst, dire->size);
      } else {
        printf("ho fatto la free\n");
        free(dst);
        break;
      }
      usleep(1);
    }
    printf("dicane sono uscito col porco cio\n");
    close(fd);
    printf("chiudo\n");
  }

  /* fd = open(fifo, O_WRONLY); */
  /* char *msg = "cioa bello"; */
  /* write(fd, msg, strlen(msg) + 1); */
  /* close(fd); */
}
