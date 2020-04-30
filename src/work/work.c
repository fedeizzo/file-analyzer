#include <stdlib.h>

#include "work.h"

Work newWork(const char *path, const int bufferStart, const int bufferEnd) {
  Work work = malloc(sizeof(Work));

  work->path = path;
  work->bufferStart = bufferStart;
  work->bufferEnd = bufferEnd;

  return work;
}

void destroyWork(Work work) { free(work); }
