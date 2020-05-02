#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../manager/manager.h"
#include "../queue/queue.h"
#include "../table/table.h"
#include "../worker/worker.h"
#include "../wrapping/wrapping.h"

#define GOOD_ENDING 0
#define BAD_ENDING -1

#define SUMMARY 0
#define NEW_DIRECTIVES 1

#define DEAD_PROCESS -1

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
#define SUMMARY_FAILURE -9
#define REMOVE_WORK_FAILURE -10
#define INIT_FAILURE -11
#define ASSIGNWORK_FAILURE -12
#define SEND_FAILURE -13

#define NCHAR_TABLE 256
#define WRITE_CHANNEL 1
#define READ_CHANNEL 0
#define MAXLEN 300
void ciccia(void *data) {
  Worker w = (Worker)data;
  /* printf("\tpid %d\n", w->pid); */
  printf("\tpid %p\n", w);
}

void pasticcio(void *data) {
  Work w = (Work)data;
  /* printf("\tpid %d\n", w->pid); */
  printf("\tpath:%s\n", w->path);
  printf("\tstart:%d\n", w->bufferStart);
  printf("\tend:%d\n", w->bufferEnd);
}

// TODO max int in C language has 10 digits. So if Naimoli insert
// file with size >= 10Gb of same char the programm will go in overflow

Worker newWorker() {
  Worker worker = malloc(sizeof(struct structWorker));
  int rc_al = checkAllocationError(worker);
  worker->table = calloc(NCHAR_TABLE, sizeof(int));
  int rc_al2 = checkAllocationError(worker->table);
  worker->pipe = malloc(2 * sizeof(int));
  int rc_al3 = checkAllocationError(worker->table);
  worker->bytesSent = 0;

  if (rc_al < 0 || rc_al2 < 0 || rc_al3 < 0)
    worker = NULL;

  return worker;
}

void destroyWorker(void *data) {
  Worker worker = (Worker)data;

  if (worker->pipe != NULL) {
    printf("pipe: %d, %d\n", worker->pipe[0], worker->pipe[1]);
    closeDescriptor(worker->pipe[0]);
    closeDescriptor(worker->pipe[1]);
  }
  free(worker->pipe);
  free(worker->table);
  free(worker->doing);
  free(worker);
}

int initManager(List workers, const int nWorkers, List tables, List todo,
                List doing, List done);
void deinitManager(List workers, List tables, List todo, List doing, List done,
                   char *path, char *lastPath);
int changeWorkersAmount(List workers, const int currentWorkers,
                        const int newWorkers, List tables, List todo,
                        List doing, List done);
int addWorkers(List workers, int amount);
int removeWorkers(List workers, int amount, List tables, List todo, List doing,
                  List done);
int executeWork(List workers, List tables, List todo, List doing, List done);
int assignWork(Worker worker, Work work, List todo, List doing);
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

int workerInitPipe(const int readPipe[], const int writePipe[]);
int parentInitExecPipe(const int readPipe[], const int writePipe[]);
int sendSummary(List tables);

int main(int argc, char *argv[]) {
  signal(SIGCHLD, SIG_IGN);
  List workers = newList();
  List tables = newList();
  List todo = newList();
  List doing = newList();
  List done = newList();

  char *path = malloc(MAXLEN * sizeof(char));
  int cl_al = checkAllocationError(path);
  char *lastPath = malloc(MAXLEN * sizeof(char));
  int cl_al2 = checkAllocationError(lastPath);
  int currentWorkers = 4;
  int newNWorker;

  int rc_work = OK;
  int rc_in = initManager(workers, currentWorkers, tables, todo, doing, done);
  int rc_nd = OK;
  int rc_wc = OK;
  int directives;

  while (rc_in == OK) {
    // TODO check this fucking sheety thing
    strcpy(lastPath, path);
    directives = readDirectives(lastPath, &currentWorkers, path, &newNWorker);
    printf("la direttiva dice: %d\n", directives);
    if (directives == SUMMARY) {
      sendSummary(tables);
    } else if (directives == NEW_DIRECTIVES) {
      if (strcmp(lastPath, path) != 0 && currentWorkers != newNWorker) {
        rc_wc = changeWorkersAmount(workers, currentWorkers, newNWorker, tables,
                                    todo, doing, done);
        currentWorkers = newNWorker;
        rc_nd = addDirectives(tables, todo, path, currentWorkers);
      } else if (strcmp(lastPath, path) != 0) {
        rc_nd = addDirectives(tables, todo, path, currentWorkers);
      } else {
        rc_wc = changeWorkersAmount(workers, currentWorkers, newNWorker, tables,
                                    todo, doing, done);
        currentWorkers = newNWorker;
      }
      if (rc_nd < OK && rc_wc < OK) {
        // TODO do something with this fucking error
        printError("ho avuto problemi con le direttive");
      }
    } else {
      printError("ho avuto problemi generici");
    }
    rc_work = executeWork(workers, tables, todo, doing, done);
    /* printList(todo, pasticcio); */
    /* printList(doing, pasticcio); */
    /* printList(done, pasticcio); */
    /* printf("size todo: %d\n", todo->size); */
    /* printf("size doing: %d\n", doing->size); */
    /* printf("size done: %d\n", done->size); */
    /* printf("size workers: %d\n", workers->size); */
    /* printf("size tables: %d\n", tables->size); */
  }
  /* deinitManager(workers, tables, todo, doing, done, path, lastPath); */
  return 0;
}

int initManager(List workers, const int nWorkers, List tables, List todo,
                List doing, List done) {
  int rc_t = OK;

  if (workers != NULL && tables != NULL && todo != NULL && doing != NULL &&
      done != NULL) {
    for (int i = 0; i < nWorkers && rc_t == OK; i++) {
      rc_t = addWorkers(workers, 1);
    }
  } else {
    rc_t = INIT_FAILURE;
  }

  return rc_t;
}

void deinitManager(List workers, List tables, List todo, List doing, List done,
                   char *path, char *lasPath) {
  destroyList(workers, destroyWorker);
  destroyList(tables, destroyTable);
  destroyList(todo, destroyWork);
  destroyList(doing, destroyWork);
  destroyList(done, destroyWork);
  free(path);
  free(lasPath);
}

int changeWorkersAmount(List workers, const int currentWorkers,
                        const int newWorkers, List tables, List todo,
                        List doing, List done) {
  int rc_t;
  int delta;
  if (currentWorkers > 0)
    delta = newWorkers - currentWorkers;
  else
    delta = newWorkers;

  if (delta > 0) {
    rc_t = addWorkers(workers, delta);
  } else {
    rc_t = removeWorkers(workers, -delta, tables, todo, doing, done);
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
      rc_en = push(workers, worker);
      if (rc_en == -1)
        rc_t = NEW_WORKER_FAILURE;
      else {
        int toParent[2];
        int toChild[2];
        createUnidirPipe(toParent);
        createUnidirPipe(toChild);
        int workerPid = fork();
        if (workerPid > 0) {
          int rc_pp = parentInitExecPipe(toParent, toChild);
          if (rc_pp == -1)
            rc_t = PIPE_FAILURE;
          else {
            worker->pid = workerPid;
            worker->pipe[0] = toParent[READ_CHANNEL];
            worker->pipe[1] = toChild[WRITE_CHANNEL];
          }
        } else {
          workerInitPipe(toParent, toChild);
          execlp("./worker", "./worker", NULL);
        }
      }
    }
  }

  return rc_t;
}

int removeWorkers(List workers, int amount, List tables, List todo, List doing,
                  List done) {
  int rc_t = OK;

  while (amount != 0 && workers->size != 0) {
    Worker w = front(workers);
    if (w != NULL) {
      int rc_po = pop(workers);
      endWork(w, tables, BAD_ENDING, todo, doing, done);
      kill(w->pid, SIGKILL);
      destroyWorker(w);
      if (rc_po == -1)
        rc_t = REMOVE_WORK_FAILURE;
      amount--;
    } else {
      rc_t = REMOVE_WORK_FAILURE;
    }
  }

  return rc_t;
}

int isAlive(Worker w) {
  int rc_t = OK;
  int returnCode = kill(w->pid, 0);
  if (returnCode != 0) {
    rc_t = DEAD_PROCESS;
  }
  return rc_t;
}

int executeWork(List workers, List tables, List todo, List doing, List done) {
  int rc_t = OK;
  int rc_po = OK;
  int rc_pu = OK;
  int rc_ww = OK;
  int workerSize = workers->size;
  int isWorkerAlive = OK;

  List newWorkers = newList();
  for (int i = 0; i < workerSize; i++) {
    Worker w;
    w = front(workers);
    isWorkerAlive = isAlive(w);
    rc_po = pop(workers);
    if (isWorkerAlive == OK) {
      rc_pu = push(newWorkers, w);
      if (rc_po == -1 && rc_pu == -1)
        rc_t = WORK_FAILURE;
      else {
        if (w != NULL) {
          if (w->doing != NULL) {
            rc_ww = getWorkerWork(w, tables, todo, doing, done);
            if (rc_ww)
              rc_t = WORK_FAILURE;
          } else {
            if (todo->size != 0) {
              Work work = front(todo);
              if (work != NULL) {
                rc_t = assignWork(w, work, todo, doing);
              } else {
                rc_t = ASSIGNWORK_FAILURE;
              }
            }
          }
        }
      }
    } else {
      destroyWorker(w);
      printf("dacci 30L ho i tuoi figli morirrano\n");
      printf("per amici di zoom che ci spiano stiamo scherzando\n");
    }
  }
  // TODO ATTENTION
  swap(workers, newWorkers);
  free(newWorkers);

  return rc_t;
}

int assignWork(Worker worker, Work work, List todo, List doing) {
  int rc_t = OK;
  int rc_po = pop(todo);
  int rc_pu = push(doing, work);
  if (rc_po != -1 && rc_pu != -1) {
    worker->doing = work;
    worker->workAmount = work->bufferEnd - work->bufferStart + 1;
    int *pipe = worker->pipe;
    char *path = malloc(MAXLEN * sizeof(char));
    int rc_al = checkAllocationError(path);
    char *bufferStart = malloc(MAXLEN * sizeof(char));
    int rc_al2 = checkAllocationError(bufferStart);
    char *bufferEnd = malloc(MAXLEN * sizeof(char));
    int rc_al3 = checkAllocationError(bufferEnd);

    if (rc_al < OK || rc_al2 < OK || rc_al3 < OK)
      rc_t = MALLOC_FAILURE;
    else {
      // TODO add check for sprintf (wrapping function)
      sprintf(path, "%s", work->path);
      sprintf(bufferStart, "%d", work->bufferStart);
      sprintf(bufferEnd, "%d", work->bufferEnd);

      int rc_wr = writeDescriptor(pipe[WRITE_CHANNEL], path);
      int rc_wr2 = writeDescriptor(pipe[WRITE_CHANNEL], bufferStart);
      int rc_wr3 = writeDescriptor(pipe[WRITE_CHANNEL], bufferEnd);

      if (rc_wr < OK || rc_wr2 < OK || rc_wr3 < OK)
        rc_t = SEND_FAILURE;

      free(path);
      free(bufferStart);
      free(bufferEnd);
    }
  } else
    rc_t = ASSIGNWORK_FAILURE;

  return rc_t;
}

int getWorkerWork(Worker w, List tables, List todo, List doing, List done) {
  int rc_t = OK;
  int readFromWorker = w->pipe[READ_CHANNEL];
  int bytesSent = w->bytesSent;
  char charSent[5];

  if (bytesSent == w->workAmount) {
    int rc_rd = readDescriptor(readFromWorker, charSent, 4);
    if (rc_rd <= 0) {
      rc_t = READ_FAILURE;
      endWork(w, tables, BAD_ENDING, todo, doing, done);
    } else {
      charSent[rc_rd] = '\0';
      if (strcmp(charSent, "done") == 0)
        endWork(w, tables, GOOD_ENDING, todo, doing, done);
      else {
        /* endWork(w, tables, BAD_ENDING, todo, doing, done); */
        rc_t = WORK_FAILURE;
      }
    }
  } else {
    int rc_rd = readDescriptor(readFromWorker, charSent, 1);
    if (rc_rd <= 0)
      rc_t = READ_FAILURE;
    else {
      // TODO think to remove this
      charSent[rc_rd] = '\0';
      int charCode = charSent[0];
      w->table[charCode] += 1;
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
    Table t2 = newTable(work->path);
    Table tFound = (Table)getData(tables, t2, compareTable);
    if (tFound != NULL) {
      updateTable(tFound->table, workerTable);

      int rc_rn = removeNode(doing, work, compareWork);
      int rc_pu = push(done, work);
      if (rc_rn == -1 || rc_pu == -1)
        rc_t = END_WORK_FAILURE;
    }
    destroyTable(t2);
  } else {
    int rc_rn = removeNode(doing, work, compareWork);
    if (work != NULL) {
      int rc_pu = push(todo, work);
      if (rc_rn == -1 || rc_pu == -1)
        rc_t = END_WORK_FAILURE;
    }
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
  char readBuffer[2] = "a";
  char nWorker[MAXLEN];

  int counter = 0;
  do {
    int rc = readChar(READ_CHANNEL, readBuffer);
    path[counter++] = readBuffer[0];
  } while (readBuffer[0] != '\0' && readBuffer[0] != '\n');
  path[counter] = '\0';
  if (path[strlen(path) - 1] == '\n') {
    path[strlen(path) - 1] = '\0';
  }

  counter = 0;
  do {
    int rc = readChar(READ_CHANNEL, readBuffer);
    nWorker[counter++] = readBuffer[0];
  } while (readBuffer[0] != '\0' && readBuffer[0] != '\n');
  nWorker[counter] = '\0';
  int rc_sc = sscanf(nWorker, "%d", newNWorker);
  if (rc_sc == 0)
    rc_t = CAST_FAILURE;

  /* printf("path: %s\n", path); */
  /* printf("lastpath: %s\n", lastPath); */
  /* printf("w: %d\n", *currentworker); */
  /* printf("new w: %d\n", *newNWorker); */

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
  } else if (strcmp(lastPath, path) == 0 && *newNWorker == *currentworker) {
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
  List todoTmp = newList();
  int rc_al2 = checkAllocationError(todoTmp);

  if (rc_al == -1 || rc_al2 == -1)
    rc_t = TABLE_FAILURE;
  else {
    int rc_pu = push(tables, t);
    if (rc_pu == 0) {
      int fd = openFile(path, O_RDONLY);
      int fileDimension = moveCursorFile(fd, 0, SEEK_END);

      if (fileDimension > 0) {
        int step = (int)fileDimension / nWorker;
        int remainder = fileDimension % nWorker;
        int rc_pu2 = OK;

        for (int i = 0; i < nWorker - 1 && rc_pu2 == OK; i++) {
          Work w = newWork(path, step * i, step * (i + 1) - 1);
          rc_pu2 = push(todoTmp, w);
        }

        if (rc_pu2 == OK) {
          // TODO per considerare anche EOF mettere solo -1
          Work w = newWork(path, step * (nWorker - 1),
                           step * nWorker + remainder - 2);
          rc_pu2 = push(todoTmp, w);
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

  // TODO create a merge function for the list
  if (rc_t == OK) {
    int rc_po = OK;
    int rc_pu = OK;
    int todoSize = todoTmp->size;
    for (int i = 0; i < todoSize; i++) {
      Work w;
      w = front(todoTmp);
      rc_po = pop(todoTmp);
      if (rc_po == -1)
        rc_t = NEW_DIRECTIVES_FAILURE;
      if (w != NULL) {
        rc_pu = push(todo, w);
        if (rc_pu == -1)
          rc_t = NEW_DIRECTIVES_FAILURE;
      }
    }
  }

  return rc_t;
}

int workerInitPipe(const int toParent[], const int toChild[]) {
  int rc_t = 0;
  int rc_cl = closeDescriptor(toChild[WRITE_CHANNEL]);
  int rc_cl2 = closeDescriptor(toParent[READ_CHANNEL]);
  int rc_du = createDup(toChild[READ_CHANNEL], 0);
  int rc_du2 = createDup(toParent[WRITE_CHANNEL], 1);
  int rc_cl3 = closeDescriptor(toChild[READ_CHANNEL]);
  int rc_cl4 = closeDescriptor(toParent[WRITE_CHANNEL]);
  if (rc_cl == -1 || rc_cl2 == -1 || rc_cl3 == -1 || rc_cl4 == -1 ||
      rc_du == -1 || rc_du2 == -1) {
    rc_t = PIPE_FAILURE;
  }

  return rc_t;
}

int parentInitExecPipe(const int toParent[], const int toChild[]) {
  int rc_t = 0;
  int rc_cl = closeDescriptor(toParent[WRITE_CHANNEL]);
  int rc_cl2 = closeDescriptor(toChild[READ_CHANNEL]);

  if (rc_cl == -1 || rc_cl2 == -1) {
    rc_t = PIPE_FAILURE;
  }
  return rc_t;
}

int sendSummary(List tables) {
  int rc_t = OK;
  int rc_po = OK;
  int rc_pu = OK;

  List newTables = newList();
  for (int i = 0; i < tables->size; i++) {
    Table t;
    t = front(tables);
    rc_po = pop(tables);
    rc_pu = push(newTables, t);
    if (rc_po == -1 || rc_pu == -1)
      rc_t = SUMMARY_FAILURE;
    if (t != NULL) {
      printf("inizion file: %s\n", t->name);
      for (int j = 0; j < NCHAR_TABLE; j++) {
        // TODO choose MAXLEN
        char msg[MAXLEN];
        int rc_sp;
        printf("%c ==>", j);
        fflush(stdout);
        rc_sp = sprintf(msg, "%d\n", t->table[j]);
        if (rc_sp == -1)
          rc_t = CAST_FAILURE;
        else {
          int rc_wr = writeDescriptor(WRITE_CHANNEL, msg);
          if (rc_wr == -1)
            rc_t = SUMMARY_FAILURE;
        }
      }
      printf("fine file: %s\n", t->name);
    }
  }
  // TODO ATTENTION
  swap(tables, newTables);
  free(newTables);

  return rc_t;
}
