#ifndef __TABLE_H__
#define __TABLE_H__

/**
 * Struct that handle a table for counting operation.
 * Is associated with a name
 *
 * fields:
 *    const char *name: name for the association
 *    int *table: array to memorize the count of the char
 */
typedef struct structTable {
  char *name;
  long long *table;
  int workAssociated;
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
 *    void *data: the table for deleting operation
 */
void destroyTable(void *data);

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
int compareTable(void *t1, void *t2);
#endif
