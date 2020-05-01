#ifndef __MANAGER_H__
#define __MANAGER_H__

#include "../work/work.h"
#include "../worker/worker.h"

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
 *    Worker worker: the worker for deleting operation
 */
void destroyWorker(Worker worker);
int workerExec(const int readPipe[], const int writePipe[], const char *exeFile);
/**
 * Sends a work to worker
 *
 * args:
 *    Work work: self explainatory
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int sendWork(Work work);
int recieveChar();
#endif
