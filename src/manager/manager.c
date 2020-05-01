#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../manager/manager.h"
#include "../queue/queue.h"
#include "../worker/worker.h"
#include "../wrapping/wrapping.h"

#define GOOD_ENDING 0
#define BAD_ENDING -1

#define SUMMARY 0
#define NEW_DIRECTIVES 1

#define OK 0
#define WORKING 1
#define NEW_WORKER_FAILURE -1
#define TABLE_FAILURE -2
#define WORK_FAILURE -3
#define READ_FAILURE -4
#define END_WORK_FAILURE -5
#define NEW_DIRECTIVES_FAILURE -6
#define CAST_FAILURE -7
#define PIPE_FAILURE -8

#define NCHAR_TABLE 256
#define WRITE_CHANNEL 1
#define READ_CHANNEL 0
#define MAXLEN 300

// TODO max int in C language has 10 digits. So if Naimoli insert
// file with size >= 10Gb of same char the programm will go in overflow

Table newTable(const char *name) {
  Table table = malloc(sizeof(Table));
  int rc_al = checkAllocationError(table);

  table->name = name;
  table->table = calloc(NCHAR_TABLE, sizeof(int));
  int rc_al2 = checkAllocationError(table->table);

  if (rc_al < 0 || rc_al2 < 0)
    table = NULL;

  return table;
}

void destroyTable(Table table) {
  free(table->table);
  free(table);
}

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
// TODO doing this inside a for that cycle amount

int initManager(List workers, List tables, List todo, List doing, List done);
int addWorkers(List workers, int amount);
int removeWorkers(List workers, int amount);
int executeWork(List workers, List tables, List todo, List doing, List done);
int getWorkerWork(Worker w, List tables, List todo, List doing, List done);
void updateTable(int *table, int *workerTable);
int endWork(Worker worker, List tables, int typeEnding, List todo, List doing,
            List done);

/**
 * Read all infos from pipe
 *
 * args:
 *    char *path: path of the file
 *    char *bufferStart: contains buffer start in chars type
 *    char *bufferEnd: contains buffer end in chars type
 */
int readDirectives(char *lastPath, int *currentworker, char *path,
                   int *newNWorker);

int addDirectives(List tables, List todo, const char *path, const int nWorker);

int workerExec(const int readPipe[], const int writePipe[],
               const char *exeFile);

int main(int argc, char *argv[]) {
  // TODO change with linked list
  List workers;
  List tables;
  List todo;
  List doing;
  List done;

  // TODO add [] for array initialization
  char *path;
  char *lastPath;
  int currentWorker;
  int newNWorker;

  int rc_work = OK;
  int rc_in = initManager(workers, tables, todo, doing, done);
  int rc_nd = OK;
  int directives;

  while (1) {
    directives = readDirectives(lastPath, &currentWorker, path, &newNWorker);
    if (directives == SUMMARY) {
      // send summary
    } else if (directives == NEW_DIRECTIVES) {
      lastPath = path;
      currentWorker = newNWorker;
      // TODO do something with this fucking error
      rc_nd = addDirectives(tables, todo, path, currentWorker);
    } else {
      // errore
    }
    /* rc_work = executeWork(workers, table); */
  }
}

int initManager(List workers, List tables, List todo, List doing, List done) {
  int rc_t = OK;
  initList(&workers);
  initList(&tables);
  initList(&todo);
  initList(&doing);
  initList(&done);
  // TODO pass nWorker as arguemnt and change
  for (int i = 0; i < workers.size && rc_t == OK; i++) {
    rc_t = addWorkers(workers, 1);
  }

  return rc_t;
}

int addWorkers(List workers, int amount) {
  int rc_t = OK;
  int rc_en = OK;
  for (int i = 0; i < amount && rc_t == OK; i++) {
    Worker worker = newWorker();
    if (worker == NULL)
      rc_t = NEW_WORKER_FAILURE;
    else {
      rc_en = push(&workers, worker);
      // TODO check this strange error
      if (rc_en == NULL)
        rc_t = NEW_WORKER_FAILURE;
    }
  }

  return rc_t;
}

int executeWork(List workers, List tables, List todo, List doing, List done) {
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
        rc_ww = getWorkerWork(w, tables, todo, doing, done);
        if (rc_ww)
          rc_t = WORK_FAILURE;
      } else {
        if (todo.size != 0) {
          /* rc_aw = assignemntwork(w) */
        }
      }
    }
  }
  workers = newWorkers;

  return rc_t;
}

int getWorkerWork(Worker w, List tables, List todo, List doing, List done) {
  int rc_t = OK;
  // TODO see which side of the pipe to use
  int *fd = w->pipe;
  int bytesSent = w->bytesSent;
  char charSent[5];

  if (bytesSent == w->workAmount) {
    int rc_rd = readDescriptor(*fd, charSent, 4);
    if (rc_rd <= 0) {
      rc_t = READ_FAILURE;
      endWork(w, tables, BAD_ENDING, todo, doing, done);
    } else {
      charSent[rc_rd] = '\0';
      if (strcmp(charSent, "done") == 0)
        endWork(w, tables, GOOD_ENDING, todo, doing, done);
      else {
        endWork(w, tables, BAD_ENDING, todo, doing, done);
        rc_t = WORK_FAILURE;
      }
    }
  } else {
    int rc_rd = readDescriptor(*fd, charSent, 1);
    if (rc_rd <= 0)
      rc_t = READ_FAILURE;
    else {
      // TODO think to remove this
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

int endWork(Worker worker, List tables, int typeEnding, List todo, List doing,
            List done) {
  int rc_t = OK;
  int rc_ca = OK;
  Work work = worker->doing;
  int *workerTable = worker->table;

  if (typeEnding == GOOD_ENDING) {
    updateTable(table, workerTable);
    // move work from doing to done
  } else {
    // move work from doing to todo
  }

  worker->bytesSent = 0;
  worker->doing = NULL;
  free(worker->table);
  worker->table = calloc(NCHAR_TABLE, sizeof(int));
  rc_ca = checkAllocationError(worker->table);
  if (rc_ca < 0)
    rc_t = END_WORK_FAILURE;

  return rc_t;
}

int readDirectives(char *lastPath, int *currentworker, char *path,
                   int *newNWorker) {
  // TODO choose max length for path
  // TODO ATTENZIONE mettere i byte terminatori nel manager
  int rc_t = SUMMARY;
  char readBuffer[1];
  char *nWorker;

  int counter = 0;
  do {
    int rc = readChar(READ_CHANNEL, readBuffer);
    path[counter++] = readBuffer[0];
  } while (readBuffer[0] != '\0' && readBuffer[0] != '\n');
  path[counter] = '\0';

  counter = 0;
  do {
    int rc = readChar(READ_CHANNEL, readBuffer);
    nWorker[counter++] = readBuffer[0];
  } while (readBuffer[0] != '\0' && readBuffer[0] != '\n');
  nWorker[counter] = '\0';
  int rc_sc = sscanf(nWorker, "%d", newNWorker);
  if (rc_sc == 0)
    rc_t = CAST_FAILURE;

  if (path[0] == '\0' || nWorker[0] == '\0') {
    char *msgErr = (char *)malloc(300);
    int rc_ca = checkAllocationError(msgErr);
    if (rc_ca < 0) {
      printError("I can't allocate memory");
    } else {
      sprintf(msgErr, "inisde worker with pid: %d", getpid());
      printError(msgErr);
      free(msgErr);
    }
  } else if (strcmp(lastPath, path) == 0 && *nWorker == *currentworker) {
    rc_t = SUMMARY;
  } else {
    rc_t = NEW_DIRECTIVES;
  }

  return rc_t;
}

int addDirectives(List tables, List todo, const char *path, const int nWorker) {
  int rc_t = OK;
  Table t = newTable(path);
  int rc_al = checkAllocationError(t);
  if (rc_al)
    rc_t = TABLE_FAILURE;
  else {
    int rc_pu = push(&tables, t);
    if (rc_pu == 0) {
      int fd = openFile(path, O_RDONLY);
      int fileDimension = moveCursorFile(fd, 0, SEEK_END);

      if (fileDimension != -1) {
        int step = (int)fileDimension / nWorker;
        int remainder = fileDimension % nWorker;
        int rc_pu2 = OK;

        for (int i = 0; i < nWorker - 1 && rc_pu2 == OK; i++) {
          Work w = newWork(path, step * i, step * (i + 1) - 1);
          rc_pu2 = push(&todo, w);
        }

        if (rc_pu2 == OK) {
          Work w =
              newWork(path, step * (nWorker - 1), step * nWorker + remainder);
          rc_pu2 = push(&todo, w);
        } else {
          rc_t = NEW_DIRECTIVES_FAILURE;
        }
      }

      int rc_cl = closeDescriptor(fd);

      if (fd == -1 || rc_cl == -1)
        rc_t = NEW_DIRECTIVES_FAILURE;
    } else {
      rc_t = NEW_DIRECTIVES_FAILURE;
    }
  }

  return rc_t;
}

int workerExec(const int readPipe[], const int writePipe[],
               const char *exeFile) {
  int rc_t = 0;
  int rc_cl = closeDescriptor(writePipe[READ_CHANNEL]);
  int rc_cl2 = closeDescriptor(readPipe[WRITE_CHANNEL]);
  int rc_du = createDup(writePipe[WRITE_CHANNEL], 1);
  int rc_du2 = createDup(readPipe[READ_CHANNEL], 0);
  int rc_cl3 = closeDescriptor(writePipe[WRITE_CHANNEL]);
  int rc_cl4 = closeDescriptor(readPipe[READ_CHANNEL]);
  if (rc_cl == -1 || rc_cl2 == -1 || rc_cl3 == -1 || rc_cl4 == -1 ||
      rc_du == -1 || rc_du2 == -1) {
    rc_t = PIPE_FAILURE;
  }

  return rc_t;
}
