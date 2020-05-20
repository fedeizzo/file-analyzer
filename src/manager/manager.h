#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <pthread.h>

#include "../list/list.h"
#include "../work/work.h"
#include "../table/table.h"
#include "../config/config.h"

/**
 * Holds directive information
 *
 * fields:
 *    char *path: path of a file
 *    char *lastPath: path of the last file
 *    int currentWorkers: current amount of worker
 *    int newNWorker: new amount of worker
 */
typedef struct DirectivesStruct {
  List paths;
  int currentWorkers;
  int newNWorker;
  int directiveStatus;
} * Directive;

typedef struct sharedResources {
  Directive directive;
  List todo;
  List workers;
  List tables;
  int summaryFlag;
  pthread_mutex_t mutex;
} sharedResources_t;

typedef struct Manager {
  int m_pid;
  int* pipe;
  List filesInExecution;
} *Manager;

/**
 * Creates a Directive
 *
 * returns:
 *    the directives created
 */
Directive newDirective();

/**
 * Destroys a Directive
 */
void destroyDirective(Directive directive);
#endif
