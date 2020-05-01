#ifndef __MANAGER_H__
#define __MANAGER_H__

#include "../work/work.h"
#include "../worker/worker.h"

/**
 * Struct that handle a table for counting operation.
 * Is associated with a name
 *
 * fields:
 *    const char *name: name for the association
 *    int *table: array to memorize the count of the char
 */
typedef struct {
  const char *name;
  int *table;
} * Table;

/**
 * Creates a new table
 *
 * args:
 *    const char *name: the name of the file (pathfile)
 *
 * returns:
 *    the table created
 */
Table newTable(const char *name);


/**
 * Deletes the table passed as argument
 *
 * args:
 *    Table table: the table for deleting operation
 */
void destroyTable(Table table);

/**
 * Compare function for search operation inside List
 *
 * args:
 *    void *t1: pointer to first table
 *    void *t2: pointer to second table
 *
 * returns:
 *    0 in caso of success, otherwise -1
 */
int compareTalbe(void *t1, void *t2);
#endif
