
#ifndef __WORK_H__
#define __WORK_H__

typedef struct {
  const char *path;
  int bufferStart;
  int bufferEnd;
} * Work;

/**
 * Creates a work
 *
 * args:
 *    const char* path: file path
 *    const int bufferStart: buffer start point
 *    const int bufferEnd: buffer end point
 *
 * returns:
 *    returs a heap allocated Work
 */
Work newWork(const char* path, const int bufferStart, const int bufferEnd);

/**
 * Destroys a work
 *
 * args:
 *    Work work: self explainatory
 */
void destroyWork(Work work);
 
#endif
