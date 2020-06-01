#include <stdlib.h>
#include <string.h>

#include "../wrapping/wrapping.h"
#include "../config/config.h"
#include "table.h"

Table newTable(const char *name) {
  // TODO if only one malloc fail we must free other
  Table table = malloc(sizeof(struct structTable));
  int rc_al = checkAllocationError(table);

  table->name = malloc(strlen(name) + 1 * sizeof(char));
  int rc_al2 = checkAllocationError(table->name);

  table->table = calloc(NCHAR_TABLE, sizeof(unsigned long long));
  int rc_al3 = checkAllocationError(table->table);

  table->workAssociated = 0;

  if (rc_al < 0 || rc_al2 < 0 || rc_al3 < 0)
    table = NULL;
  else
    strcpy(table->name, name);

  return table;
}

void destroyTable(void *data) {
  Table table = (Table)data;
  free(table->name);
  free(table->table);
  free(table);
}

int compareTable(void *t1, void *t2) {
  int rc_t = -1;
  Table table1 = (Table)t1;
  Table table2 = (Table)t2;

  if (strcmp(table1->name, table2->name) == 0 && table1->workAssociated == table2->workAssociated) {
    rc_t = 0;
  }
  return rc_t;
}
