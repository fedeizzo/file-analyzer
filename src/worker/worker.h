#ifndef __WORKER_H__
#define __WORKER_H__

#include "../work/work.h"

/**
 * Handles a worker. A worker is a associated with:
 *    * a pid for eventually kill operation
 *    * a table for counting opeartion
 *    * bytes sent from the worker to the manager
 *    * remaing work of the worker
 *    * a work under execution
 *    * a pipe for the communication
 *
 * fields:
 *    int pid: the pid
 *    int *table: the table for the counting operation
 *    int bytesSent: the amount of bytes sent
 *    const int workAmount: the remaining work
 *    Work doing: the work under execution
 *    int *pipe: the pipe for the communication with the manager
 */
typedef struct structWorker{
  int pid;
  long long *table;
  int bytesSent;
  int workAmount;
  Work doing;
  int *pipe;
} * Worker;

/**
 * Creates a new worker
 *
 * returns:
 *    the created worker
 */
Worker newWorker();

/**
 * Deletes the worker passed as argument
 *
 * args:
 *    void *data: the worker for deleting operation
 */
void destroyWorker(void *data);
#endif 
