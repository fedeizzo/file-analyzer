#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../queue/queue.h"
#include "../worker/worker.h"
#include "../wrapping/wrapping.h"

#define GOOD_ENDING 0
#define BAD_ENDING -1

#define OK 0
#define WORKING 1
#define NEW_WORKER_FAILURE -1
#define TABLE_FAILURE -2
#define WORK_FAILURE -3
#define READ_FAILURE -4
#define END_WORK_FAILURE -5

#define CAST_FAILURE -2
#define READ_DIRECTIVES_FAILURE -3
#define CURSOR_FAILURE -4
#define WRITE_FAILURE -6

#define NCHAR_TABLE 256
#define WRITE_CHANNEL 1
#define READ_CHANNEL 0
#define MAXLEN 300

Worker newWorker() {
  Worker worker = malloc(sizeof(worker));
  int rc_al = checkAllocationError(worker);
  worker->table = calloc(NCHAR_TABLE, sizeof(int));
  int rc_al2 = checkAllocationError(worker->table);
  worker->bytesSent = 0;

  if (rc_al < 0 || rc_al2 < 0)
    worker = NULL;

  return worker;
}

void destroyWorker(Worker worker) {
  free(worker->table);
  free(worker->doing);
  free(worker);
}

int initManager(List workers, int *table);
int addWorkers(List workers, int amount);
int removeWorkers(List workers, int amount);
int executeWork(List workers, int *table);
int getWorkerWork(Worker w, int *table);
void updateTable(int *table, int *workerTable);
int endWork(Worker worker, int *table, int typeEnding);

/* int workerExec(const int readPipe[], const int writePipe[], */
/*                const char *exeFile) { */
/*   int rc_t = 0; */
/*   // TODO change close to close descriptor */
/*   int rc_cl = close(writePipe[READ]); */
/*   int rc_cl2 = close(readPipe[WRITE]); */
/*   // TODO create wrapping function for dup2 */
/*   int rc_du = dup2(writePipe[WRITE], 0); */
/*   int rc_du2 = dup2(readPipe[READ], 1); */
/*   int rc_cl3 = close(writePipe[WRITE]); */
/*   int rc_cl4 = close(readPipe[READ]); */
/*   if (rc_cl == -1 || rc_cl2 == -1 || rc_cl3 == -1 || rc_cl4 == -1 || */
/*       rc_du == -1 || rc_du2 == -1) { */
/*     rc_t = -1; */
/*   } */

int main(int argc, char *argv[]) {
  // TODO change with linked list
  List workers;
  int *table;
  int rc_work = OK;
  int rc_in = initManager(workers, table);

  while (1) {
    int rc_work = executeWork(workers, table);
  }
}

int initManager(List workers, int *table) {
  int rc_t = OK;
  initList(&workers);
  for (int i = 0; i < workers.size && rc_t == OK; i++) {
    rc_t = addWorkers(workers, 1);
  }

  table = calloc(NCHAR_TABLE, sizeof(int));
  int rc_al = checkAllocationError(table);
  if (rc_al)
    rc_t = TABLE_FAILURE;

  return rc_t;
}

int addWorkers(List workers, int amount) {
  // TODO doing this inside a for that cycle amount
  int rc_t = OK;
  int rc_en = OK;
  Worker worker = newWorker();
  if (worker == NULL)
    rc_t = NEW_WORKER_FAILURE;
  else {
    rc_en = push(&workers, newWorker());
    // TODO check this strange error
    if (rc_en == NULL)
      rc_t = NEW_WORKER_FAILURE;
  }

  return rc_t;
}

int executeWork(List workers, int *table) {
  int rc_t = OK;
  int rc_po = OK;
  int rc_pu = OK;
  int rc_ww = OK;

  List newWorkers;
  initList(&newWorkers);
  for (int i = 0; i < workers.size; i++) {
    Worker w;
    w = front(&workers);
    rc_po = pop(&workers);
    rc_pu = push(&newWorkers, w);
    // TODO say to samuele bool propblem:
    // he uses 1 for true and 0 for false
    // but in all other code 0 stands
    // for correct case (also all c standard function)
    if (rc_po == 0 && rc_pu == NULL)
      rc_t = WORK_FAILURE;
    else {
      if (w->doing != NULL) {
        rc_ww = getWorkerWork(w, table);
        if (rc_ww)
          rc_t = WORK_FAILURE;
      }
    }
  }

  return rc_t;
}

int getWorkerWork(Worker w, int *table) {
  int rc_t = OK;
  int fd = w->pipe;
  int bytesSent = w->bytesSent;
  char charSent[2];

  if (bytesSent == w->workAmount) {
    int rc_rd = readDescriptor(fd, charSent, 4);
    if (rc_rd <= 0) {
      rc_t = READ_FAILURE;
      endWork(w, table, BAD_ENDING);
    } else {
      // controllare se parola di controllo e' erro oppure done
    }
  } else {
    int rc_rd = readDescriptor(fd, charSent, 1);
    if (rc_rd <= 0)
      rc_t = READ_FAILURE;
    else {
      charSent[rc_rd] = '\0';
      int charCode = charSent[0];
      w->table[charCode]++;
      w->bytesSent++;
    }
  }
  return rc_t;
}

void updateTable(int *table, int *workerTable) {
  for (int i = 0; i < NCHAR_TABLE; i++) {
    table[i] += workerTable[i];
  }
}

int endWork(Worker worker, int *table, int typeEnding) {
  int rc_t = OK;
  int rc_ca = OK;
  Work work = worker->doing;
  int *workerTable = worker->table;

  if (typeEnding == GOOD_ENDING) {
    updateTable(table, workerTable);
  } else {
    // move work from doing to todo
  }

  worker->bytesSent = 0;
  worker->doing = NULL;
  free(worker->table);
  worker->table = calloc(NCHAR_TABLE, sizeof(int));
  rc_ca = checkAllocationError(worker->table);
  if (rc_ca < 0) {
    rc_t = END_WORK_FAILURE;
  }

  return rc_t;
}
