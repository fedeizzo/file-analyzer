#include <stdlib.h>
#include <string.h>

#include "../wrapping/wrapping.h"
#include "work.h"

Work newWork(const char *path, const int bufferStart, const int bufferEnd) {
  Work work = malloc(sizeof(Work));
  int rc_al = checkAllocationError(work);

  work->path = malloc(strlen(path) + 1 * sizeof(char));
  int rc_al2 = checkAllocationError(work->path);
  work->bufferStart = bufferStart;
  work->bufferEnd = bufferEnd;

  if (rc_al == -1 || rc_al2 == -1)
    return NULL;
  else
    strcpy(work->path, path);

  return work;
}

void destroyWork(void *data) {
  Work work = (Work)data;
  free(work->path);
  free(work);
}

int compareWork(void *w1, void *w2) {
  int rc_t = -1;

  Work work1 = (Work)w1;
  Work work2 = (Work)w2;

  if (strcmp(work1->path, work2->path) == 0 && work1->bufferEnd == work2->bufferEnd && work1->bufferStart == work2->bufferStart)
    rc_t = 0;

  return rc_t;
}
