#ifndef __WORKER_H__
#define __WORKER_H__

#include "../work/work.h"

// dentro worker ci sara' una tabella
// se a fine work il worker comunica al manager
// che tutto e' ok il manager aggiorna la propria tabella
// aggiungendo i valori di qeulla del worker.

// il manager mantiene tre array:
// * todo
// * doing
// * done
// se successe qualcosa di imprevisto vengono
// cancellate le tabelle dei worker e tutti i work
// in doing vengono spostati in todo
typedef struct {
  int pid;
  int *table;
  int bytesSent;
  const int workAmount;
  Work doing;
  int pipe[];
} * Worker;


  // MACROSTRUUTRA
/* while true */
/*   for w in worker */
/*     p = w.pipe */
/*     if (bytesSent == workAmount) */
/*       commreadDes(p, 4) */
/*     else */
/*       c = readDes(p) */
/*       w.table[c]++ */
/*       w.bytesSent++ */
/*   endfor */
/*   sendToAnalyzer() */

int receiveWork();
int sendChar(const char msg);
int endWork();
#endif 
