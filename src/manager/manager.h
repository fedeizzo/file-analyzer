#ifndef __MANAGER_H__
#define __MANAGER_H__

#include "../work/work.h"

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
