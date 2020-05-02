#include <stdlib.h>
#include <string.h>

#include "work.h"

Work newWork(const char *path, const int bufferStart, const int bufferEnd) {
  Work work = malloc(sizeof(Work));

  work->path = path;
  work->bufferStart = bufferStart;
  work->bufferEnd = bufferEnd;

  return work;
}

void destroyWork(void *data) {
  Work work = (Work)data;
  free(work);
}

int compareWork(void *w1, void *w2) {
  int rc_t = -1;

  Work work1 = (Work)w1;
  Work work2 = (Work)w2;

  if (strcmp(work1->path, work2->path) == 0)
    rc_t = 0;

  return rc_t;
}
