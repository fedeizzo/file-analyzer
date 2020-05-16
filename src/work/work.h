#ifndef __WORK_H__
#define __WORK_H__

#include "../table/table.h"

/**
 * Handles a work. A work is a associated with:
 *    * a file
 *    * a start poisition from which worker start reading
 *    * a end position in which worker end reading
 *
 * fields:
 *    Talbe tablePointer: pointer to the table inside the list of tables in manger
 *    const char *path: the path of the file
 *    int bufferStart: the start position
 *    int bufferEnd: the end position
 */
typedef struct WorkStr {
  Table tablePointer;
  int bufferStart;
  int bufferEnd;
} * Work;

/**
 * Creates a work
 *
 * args:
 *    Table table: table pointer
 *    const char* path: file path
 *    const int bufferStart: buffer start point
 *    const int bufferEnd: buffer end point
 *
 * returns:
 *    returs a heap allocated Work
 */
Work newWork(Table table, const int bufferStart, const int bufferEnd);

/**
 * Compare function for search operation inside List
 *
 * args:
 *    void *t1: pointer to first work
 *    void *t2: pointer to second work
 *
 * returns:
 *    0 in caso of success, otherwise -1
 */
int compareWork(void *w1, void *w2);

/**
 * Destroys a work
 *
 * args:
 *    void *data: self explainatory
 */
void destroyWork(void *data);
#endif
