#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../list/list.h"
#include "../manager/manager.h"
#include "../table/table.h"
#include "../worker/worker.h"
#include "../wrapping/wrapping.h"

#define GOOD_ENDING 0
#define BAD_ENDING -1

#define SUMMARY 0
#define NEW_DIRECTIVES 1

#define HARAKIRI -1

#define OK 0
#define WORKING 1
#define NEW_WORKER_FAILURE -1
#define TABLE_FAILURE -50
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
#define DESCRIPTOR_FAILURE -14
#define ASSIGNWORK_MEMORY_FAILURE -15
#define DEAD_PROCESS -16
#define START_NEW_MANAGER -17

#define NCHAR_TABLE 128
#define WRITE_CHANNEL 1
#define READ_CHANNEL 0
#define MAXLEN 300

// TODO max int in C language has 10 digits. So if Naimoli insert
// file with size >= 10Gb of same char the programm will go in overflow
//
// TODO see const arguments to function

Worker newWorker() {
  int rc_al = OK;
  int rc_al2 = OK;
  int rc_al3 = OK;
  Worker worker = malloc(sizeof(struct structWorker));
  rc_al = checkAllocationError(worker);
  if (rc_al == OK) {
    worker->table = calloc(NCHAR_TABLE, sizeof(long long));
    rc_al2 = checkAllocationError(worker->table);
    worker->pipe = malloc(2 * sizeof(int));
    rc_al3 = checkAllocationError(worker->table);
    worker->bytesSent = 0;
    worker->doing = NULL;
  }

  if (rc_al < 0 || rc_al2 < 0 || rc_al3 < 0)
    worker = NULL;
  return worker;
}

void destroyWorker(void *data) {
  Worker worker = (Worker)data;

  // TODO find a way to check return code
  // there is a problem because this function
  // must maintain this signature for removal operation of list
  if (worker->pipe != NULL) {
    closeDescriptor(worker->pipe[0]);
    closeDescriptor(worker->pipe[1]);
  }
  /* if (worker->doing != NULL) { */
  /*   destroyWork(worker->doing); */
  /* } */
  free(worker->pipe);
  free(worker->table);
  /* free(worker->doing); */
  free(worker);
}

int compareWorker(void *w1, void *w2) {
  int rc_t = -1;
  Worker work1 = (Worker)w1;
  Worker work2 = (Worker)w2;

  if (work1->pid == work2->pid) {
    rc_t = 0;
  }

  return rc_t;
}

Directive newDirective() {
  int rc_al = OK;
  int rc_al2 = OK;
  int rc_al3 = OK;
  Directive directive = malloc(sizeof(struct DirectivesStruct));
  rc_al = checkAllocationError(directive);

  if (rc_al == OK) {
    directive->path = malloc(MAXLEN * sizeof(char));
    rc_al2 = checkAllocationError(directive);
    directive->lastPath = malloc(MAXLEN * sizeof(char));
    rc_al3 = checkAllocationError(directive);
    directive->currentWorkers = 4;
    directive->directiveStatus = START_NEW_MANAGER;
  }

  if (rc_al < 0 || rc_al2 < 0 || rc_al3 < 0)
    directive = NULL;

  return directive;
}

void destroyDirective(Directive directive) {
  free(directive->path);
  free(directive->lastPath);
  free(directive);
}

/**
 * Initializes manager
 *
 * args:
 *    List workers: workers list
 *    const int nWorkers: number of workers
 *    List tables: tables list
 *    List todo: todo works list
 *    List doing: doing works list
 *    List done: done work list
 *
 * returns
 *    0 in case of success, negative number otherwise
 */
int initManager(List workers, const int nWorkers, List tables, List todo,
                List doing, List done);

/**
 * Deinitializes manger
 *
 * args:
 *    List workers: workers list
 *    List tables: tables list
 *    List todo: todo works list
 *    List doing: doing works list
 *    List done: done works list
 *    char *path: path to file
 *    char *lastPath: last path sent in STDIN
 *
 * returns
 *    0 in case of success, negative number otherwise
 */
void deinitManager(List workers, List tables, List todo, List doing, List done,
                   char *path, char *lastPath);

/**
 * Changes workers amount under manager control
 *
 * args:
 *    List workers: workers list
 *    const int currentWorkers: number of current workers
 *    const int newWorkers: number of new workers
 *    List tables: tables list
 *    List todo: todo works list
 *    List doing: doing works list
 *    LIst done: done works list
 *
 * returns
 *    0 in case of success, negative number otherwise
 */
int changeWorkersAmount(List workers, const int currentWorkers,
                        const int newWorkers, List tables, List todo,
                        List doing, List done);

/**
 * Adds worker
 *
 * args:
 *    List workers: workers list
 *    int amount: worker amount
 *
 * returns
 *    0 in case of success, negative number otherwise
 */
int addWorkers(List workers, int amount);

/**
 * Removes manger
 *
 * args:
 *    List workers: workers list
 *    int amount: workeer amount
 *    List tables: tables list
 *    List todo: todo works list
 *    List doing: doing works list
 *    List done: done works list
 *
 * returns
 *    0 in case of success, negative number otherwise
 */
int removeWorkers(List workers, int amount, List tables, List todo, List doing,
                  List done);

/**
 * Executes the manger manager
 *
 * args:
 *    List workers: workers list
 *    List tables: tables list
 *    List todo: todo works list
 *    List doing: doing works list
 *    List done: done works list
 *
 * returns
 *    0 in case of success, negative number otherwise
 */
int executeWork(List workers, List tables, List todo, List doing, List done);

/**
 * Assigns work to worker
 *
 * args:
 *    Worker worker: the worker that recives the work
 *    Work work: the work that is assigned
 *    List todo: todo works list
 *    List doing: doing works list
 *
 * returns
 *    0 in case of success, negative number otherwise
 */
int assignWork(Worker worker, Work work, List todo, List doing);

/**
 * Gets chars from workers
 *
 * args:
 *    List workers: workers list
 *    List tables: tables list
 *    List todo: todo works list
 *    List doing: doing works list
 *    List done: done works list
 *
 * returns
 *    0 in case of success, negative number otherwise
 */
int getWorkerWork(Worker w, List tables, List todo, List doing, List done);

/**
 * Checks if a work pass from doint to done. If so sends summary is called
 *
 * args:
 *    int *workDone: last amount of done works
 *    List done: list of work to done for the comparison
 *
 * returns:
 *    0 in case there are new work in done list, -1 otherwise
 */
int checkUpdate(int *workDone, List done);

/**
 * Updates manger table
 *
 * args:
 *    int *table: the table associated with a file
 *    int *workerTable: the worker table for the udpate operation
 */
void updateTable(long long *table, long long *workerTable);

/**
 * Ends a worker's work
 *
 * args:
 *    Worker worker: the worker which has ended work
 *    List tables: tables list
 *    int typeEnding: the ending type
 *    List todo: todo works list
 *    List doing: doing works list
 *    List done: done works list
 *
 * returns
 *    0 in case of success, negative number otherwise
 */
int endWork(Worker worker, List tables, int typeEnding, List todo, List doing,
            List done);

/**
 * Reads all infos from pipe
 *
 * args:
 *    char *path: path of the file
 *    char *bufferStart: contains buffer start in chars type
 *    char *bufferEnd: contains buffer end in chars type
 */
void *readDirectives(void *ptr);

/**
 * Adds directives in todo list
 *
 * args:
 *    List tables: tables list
 *    List todo: todo works list
 *    char *path: path to file
 *    const int nWorker: number of workers used to creates works
 *
 * returns
 *    0 in case of success, negative number otherwise
 */
int addDirectives(List tables, List todo, const char *path, const int nWorker);

/**
 * Inits worker's pipes
 *
 * args:
 *    const int readPipe[]: read pipe
 *    const int writePipe[]: write pipe
 *
 * returns
 *    0 in case of success, negative number otherwise
 */
int workerInitPipe(const int readPipe[], const int writePipe[]);

/**
 * Inits manger's pipe
 *
 * args:
 *    const int readPipe[]: read pipe
 *    const int writePipe[]: write pipe
 *
 * returns
      printf("nWorker: %d\n", rc_t);
 *    0 in case of success, negative number otherwise
 */
int parentInitExecPipe(const int readPipe[], const int writePipe[]);

/**
 * Sends summary to STDOUT
 *
 * args:
 *    List workers: workers list
 *    List tables: tables list
 *    List todo: todo works list
 *    List doing: doing works list
 *    List done: done works list
 *    char *path: path to file
 *    char *lastPath: last path sent in STDIN
 *
 * returns
 *    0 in case of success, negative number otherwise
 */
int sendSummary(List tables);

/**
 * Handles errors
 *
 * args:
 *    int errorCode: error code
 *
 * returns
 *    0 in case of success input error is no fatal, otherwise -1
 */
int errorHandler(int errorCode);

/**
 * Work loop for thread
 *
 * args:
 *    void *ptr: args for thread
 */
void *workLoop(void *ptr);

int main(int argc, char *argv[]) {
  signal(SIGCHLD, SIG_IGN);
  pthread_t directives, work;
  int iret1, iret2;
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

  int rc_work = initManager(workers, currentWorkers, tables, todo, doing, done);

  sharedResources_t sharedResourses;
  sharedResourses.directive = newDirective();
  pthread_mutex_init(&sharedResourses.mutex, NULL);
  sharedResourses.todo = todo;
  sharedResourses.doing = doing;
  sharedResourses.done = done;
  sharedResourses.workers = workers;
  sharedResourses.tables = tables;

  if (rc_work == OK) {
    iret1 = pthread_create(&directives, NULL, readDirectives,
                           (void *)&sharedResourses);
    iret2 = pthread_create(&work, NULL, workLoop, (void *)&sharedResourses);

    pthread_join(directives, NULL);
    pthread_join(work, NULL);
  }

  // TODO free of directives inside aharedResources
  deinitManager(workers, tables, todo, doing, done, path, lastPath);
  destroyDirective(sharedResourses.directive);
  // return rc_work;
}

void *workLoop(void *ptr) {
  sharedResources_t *sharedRes = (sharedResources_t *)(ptr);

  int rc_work = OK;
  int rc_nd = OK;
  int rc_wc = OK;
  int directives;
  int workDone = 0;

  while (rc_work == OK) {
    pthread_mutex_lock(&(sharedRes->mutex));
    directives = sharedRes->directive->directiveStatus;
    pthread_mutex_unlock(&(sharedRes->mutex));
    while (directives == START_NEW_MANAGER) {
      pthread_mutex_lock(&(sharedRes->mutex));
      directives = sharedRes->directive->directiveStatus;
      pthread_mutex_unlock(&(sharedRes->mutex));
      usleep(500000);
    }
    if (directives == SUMMARY) {
      if (checkUpdate(&workDone, sharedRes->done) == SUMMARY) {
        pthread_mutex_lock(&(sharedRes->mutex));
        sendSummary(sharedRes->tables);
        pthread_mutex_unlock(&(sharedRes->mutex));
      }
    } else if (directives == NEW_DIRECTIVES) {
      pthread_mutex_lock(&(sharedRes->mutex));
      if (strcmp(sharedRes->directive->lastPath, sharedRes->directive->path) !=
              0 &&
          sharedRes->directive->currentWorkers !=
              sharedRes->directive->newNWorker) {
        rc_wc = changeWorkersAmount(
            sharedRes->workers, sharedRes->directive->currentWorkers,
            sharedRes->directive->newNWorker, sharedRes->tables,
            sharedRes->todo, sharedRes->doing, sharedRes->done);
        sharedRes->directive->currentWorkers = sharedRes->directive->newNWorker;
        // TODO check where to assign path to lastpath
        strcpy(sharedRes->directive->lastPath, sharedRes->directive->path);
        rc_nd = addDirectives(sharedRes->tables, sharedRes->todo,
                              sharedRes->directive->path,
                              sharedRes->directive->currentWorkers);
      } else if (strcmp(sharedRes->directive->lastPath,
                        sharedRes->directive->path) != 0) {
        // TODO check where to assign path to lastpath
        strcpy(sharedRes->directive->lastPath, sharedRes->directive->path);
        rc_nd = addDirectives(sharedRes->tables, sharedRes->todo,
                              sharedRes->directive->path,
                              sharedRes->directive->currentWorkers);
      } else {
        rc_wc = changeWorkersAmount(
            sharedRes->workers, sharedRes->directive->currentWorkers,
            sharedRes->directive->newNWorker, sharedRes->tables,
            sharedRes->todo, sharedRes->doing, sharedRes->done);
        sharedRes->directive->currentWorkers = sharedRes->directive->newNWorker;
      }
      sharedRes->directive->directiveStatus = SUMMARY;
      pthread_mutex_unlock(&(sharedRes->mutex));
      if (rc_nd < OK && rc_wc < OK) {
        int r1 = errorHandler(rc_nd);
        int r2 = errorHandler(rc_wc);
        if (r1 != OK || r2 != OK)
          rc_work = HARAKIRI;
      } else if (rc_nd < OK) {
        rc_work = errorHandler(rc_nd);
      } else if (rc_wc < OK) {
        rc_work = errorHandler(rc_wc);
      }
    } else {
      rc_work = errorHandler(directives);
    }
    if (rc_work == OK) {
      pthread_mutex_lock(&(sharedRes->mutex));
      rc_work = executeWork(sharedRes->workers, sharedRes->tables,
                            sharedRes->todo, sharedRes->doing, sharedRes->done);
      pthread_mutex_unlock(&(sharedRes->mutex));
      if (rc_work < OK)
        rc_work = errorHandler(rc_work);
    }
    // TODO fix sleep time
    usleep(1);
  }
  fprintf(stderr, "MANAGER ---- ERRORE MAGGICO %d\n", rc_work);
  printError("sono uscito magicamente");
  kill(getpid(), SIGKILL);

  // TODO error handling
}

int initManager(List workers, const int nWorkers, List tables, List todo,
                List doing, List done) {
  int rc_t = OK;

  if (workers != NULL && tables != NULL && todo != NULL && doing != NULL &&
      done != NULL) {
    int i = 0;
    for (i = 0; i < nWorkers && rc_t == OK; i++) {
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
  int i = 0;
  for (i = 0; i < amount && rc_t == OK; i++) {
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
        fcntl(toParent[0], F_SETFL, O_NONBLOCK);
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
  int i = 0;
  for (i = 0; i < workerSize; i++) {
    Worker w;
    w = front(workers);
    if (w != NULL) {
      isWorkerAlive = isAlive(w);
      rc_po = pop(workers);
      if (isWorkerAlive == OK) {
        rc_pu = push(newWorkers, w);
        if (rc_po == -1 && rc_pu == -1)
          rc_t = NEW_WORKER_FAILURE;
        else {
          if (w->doing != NULL) {
            rc_ww = getWorkerWork(w, tables, todo, doing, done);
            if (rc_ww < OK)
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
      } else {
        // TODO find better way to do this
        if (w->doing != NULL) {
          int rc_re = removeNode(doing, w->doing, compareWork);
          rc_pu = push(todo, w->doing);
          if (rc_pu < OK || rc_re < OK)
            rc_t = MALLOC_FAILURE;
          else {
            destroyWorker(w);
            addWorkers(newWorkers, 1);
            rc_t = DEAD_PROCESS;
          }
        } else {
          destroyWorker(w);
          addWorkers(newWorkers, 1);
          rc_t = DEAD_PROCESS;
        }
      }
    }
  }
  // TODO ATTENTION
  swap(workers, newWorkers);
  /* printError("new worker in execute work"); */
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
    rc_t = ASSIGNWORK_MEMORY_FAILURE;

  return rc_t;
}

int getWorkerWork(Worker w, List tables, List todo, List doing, List done) {
  int rc_t = OK;
  int readFromWorker = w->pipe[READ_CHANNEL];
  int bytesSent = w->bytesSent;
  char *charSent = malloc(w->workAmount * sizeof(char));
  int rc_al = checkAllocationError(charSent);
  if (rc_al < OK)
    rc_t = MALLOC_FAILURE;
  if (rc_t == OK) {
    if (bytesSent == w->workAmount) {
      /* int rc_rd = readDescriptor(readFromWorker, charSent, 5); */
      int rc_rd = read(readFromWorker, charSent, 5);
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
      /* int rc_rd = readDescriptor(readFromWorker, charSent, */
      /*                            w->workAmount - w->bytesSent); */
      int rc_rd = read(readFromWorker, charSent, w->workAmount - w->bytesSent);
      if (rc_rd <= 0)
        rc_t = READ_FAILURE;
      else {
        // TODO think to remove this
        /* charSent[rc_rd] = '\0'; */
        int i = 0;
        for (i = 0; i < rc_rd; i++) {
          int charCode = charSent[i];

          if (charCode < 128 && charCode > 0) {
            w->table[charCode] += 1;
          }
          w->bytesSent++;
        }
      }
    }
    free(charSent);
  }
  return rc_t;
}

int checkUpdate(int *workDone, List done) {
  int rc_t = -1;
  if (done->size > *workDone) {
    *workDone = done->size;
    rc_t = SUMMARY;
  }
  return rc_t;
}

void updateTable(long long *table, long long *workerTable) {
  int i = 0;
  for (i = 0; i < NCHAR_TABLE; i++) {
    table[i] += workerTable[i];
  }
}

int endWork(Worker worker, List tables, int typeEnding, List todo, List doing,
            List done) {
  int rc_t = OK;
  int rc_ca = OK;
  Work work = worker->doing;
  long long *workerTable = worker->table;

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
  int i = 0;
  for (i = 0; i < NCHAR_TABLE; i++) {
    worker->table[i] = 0;
  }

  return rc_t;
}

void *readDirectives(void *ptr) {
  sharedResources_t *sharedRes = (sharedResources_t *)(ptr);
  // TODO choose max length for path
  // TODO ATTENZIONE mettere i byte terminatori nel manager
  int rc_t = OK;
  char readBuffer[2] = "a";
  char *newPath = malloc(MAXLEN * sizeof(char));
  int rc_al = checkAllocationError(newPath);
  if (rc_al < OK) {
    rc_t = MALLOC_FAILURE;
  }
  char nWorker[MAXLEN];
  int counter = 0;

  while (rc_t == OK) {
    counter = 0;
    do {
      int rc = readChar(READ_CHANNEL, readBuffer);
      newPath[counter++] = readBuffer[0];
    } while (readBuffer[0] != '\0' && readBuffer[0] != '\n');
    newPath[counter] = '\0';
    if (newPath[strlen(newPath) - 1] == '\n') {
      newPath[strlen(newPath) - 1] = '\0';
    }

    counter = 0;
    do {
      int rc = readChar(READ_CHANNEL, readBuffer);
      nWorker[counter++] = readBuffer[0];
    } while (readBuffer[0] != '\0' && readBuffer[0] != '\n');
    nWorker[counter] = '\0';

    pthread_mutex_lock(&(sharedRes->mutex));
    int rc_sc = sscanf(nWorker, "%d", &(sharedRes->directive->newNWorker));
    if (rc_sc == 0 ||
        (sharedRes->directive->newNWorker == 9 && strcmp(nWorker, "9") != 0)) {
      rc_t = CAST_FAILURE;
    }
    strcpy(sharedRes->directive->path, newPath);

    if (newPath[0] == '\0' || nWorker[0] == '\0') {
      char *msgErr = (char *)malloc(300);
      int rc_ca = checkAllocationError(msgErr);
      if (rc_ca < 0) {
        printError("I can't allocate memory");
      } else {
        sprintf(msgErr, "inisde worker with pid: %d", getpid());
        printError(msgErr);
        free(msgErr);
      }
    } else if (strcmp(sharedRes->directive->lastPath,
                      sharedRes->directive->path) == 0 &&
               sharedRes->directive->newNWorker ==
                   sharedRes->directive->currentWorkers) {
      sharedRes->directive->directiveStatus = SUMMARY;
    } else if (rc_t == OK) {
      sharedRes->directive->directiveStatus = NEW_DIRECTIVES;
    }
    // Da togliere
    fprintf(stderr, "MANAGER --- Path: %s\n", sharedRes->directive->path);
    fprintf(stderr, "MANAGER --- Nworker: %d\n",
            sharedRes->directive->newNWorker);
    pthread_mutex_unlock(&(sharedRes->mutex));
    // TODO fix sleep time
    usleep(500000);
  }

  printError("sono uscito magicamente 2");
  free(newPath);
  kill(getpid(), SIGKILL);
  // return rc_t;
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

      if (fileDimension > 0 && nWorker > 0) {
        int step = (int)fileDimension / nWorker;
        int remainder = fileDimension % nWorker;
        int rc_pu2 = OK;

        int i = 0;
        for (i = 0; i < nWorker - 1 && rc_pu2 == OK; i++) {
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
      } else {
        pop(tables);
      }

      int rc_cl = closeDescriptor(fd);

      if (fd == -1 || rc_cl == -1)
        rc_t = DESCRIPTOR_FAILURE;
    } else {
      rc_t = NEW_DIRECTIVES_FAILURE;
    }
  }

  // TODO create a merge function for the list
  if (rc_t == OK) {
    int rc_po = OK;
    int rc_pu = OK;
    int todoSize = todoTmp->size;
    int i = 0;
    for (i = 0; i < todoSize; i++) {
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
  free(todoTmp);

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
  int tablesSize = tables->size;

  List newTables = newList();
  int i = 0;
  for (i = 0; i < tablesSize; i++) {
    Table t;
    t = front(tables);
    rc_po = pop(tables);
    rc_pu = push(newTables, t);
    if (rc_po == -1 || rc_pu == -1)
      rc_t = SUMMARY_FAILURE;
    if (t != NULL) {
      int j = 0;
      long long acc = 0;
      for (j = 0; j < NCHAR_TABLE; j++) {
        // TODO choose MAXLEN
        char msg[MAXLEN];
        int rc_sp;
        rc_sp = sprintf(msg, "%lld", t->table[j]);
        acc += t->table[j];
        if (rc_sp == -1)
          rc_t = CAST_FAILURE;
        else {
          /* int rc_wr = writeDescriptor(WRITE_CHANNEL, msg); */
          /* if (rc_wr == -1) */
          /*   rc_t = SUMMARY_FAILURE; */
        }
      }
      fprintf(stderr, "acc: %lld\n", acc);
    }
  }
  // TODO ATTENTION
  swap(tables, newTables);
  free(newTables);

  return rc_t;
}

int errorHandler(int errorCode) {
  int rc_t = OK;

  switch (errorCode) {
  case NEW_WORKER_FAILURE:
    printError("no memory for worker creation");
    rc_t = HARAKIRI;
    break;
  case TABLE_FAILURE:
    printError("no memory for table creation");
    rc_t = HARAKIRI;
    break;
  case WORK_FAILURE:
    /* printInfo("worker doesn't execute his work"); */
    rc_t = OK;
    break;
  case READ_FAILURE:
    /* printInfo("reading from worker"); */
    rc_t = OK;
    break;
  case END_WORK_FAILURE:
    printError("no memory for work movement");
    rc_t = HARAKIRI;
    break;
  case NEW_DIRECTIVES_FAILURE:
    printError("no memory for new directives");
    rc_t = HARAKIRI;
    break;
  case CAST_FAILURE:
    printInfo("worker number must be integer");
    rc_t = OK;
    break;
  case PIPE_FAILURE:
    printInfo("no communication pipe between worker and manager");
    rc_t = OK;
    break;
  case SUMMARY_FAILURE:
    printError("no memory for new tables or no communication with parent");
    rc_t = HARAKIRI;
    break;
  case REMOVE_WORK_FAILURE:
    printInfo("no worker to remove");
    rc_t = OK;
    break;
  case INIT_FAILURE:
    printError("no memory to start manager process");
    rc_t = HARAKIRI;
    break;
  case ASSIGNWORK_FAILURE:
    printInfo("no work to assign");
    rc_t = OK;
    break;
  case SEND_FAILURE:
    printInfo("not able to send work to worker");
    rc_t = OK;
    break;
  case ASSIGNWORK_MEMORY_FAILURE:
    printError("no memory to assign work");
    rc_t = HARAKIRI;
    break;
  case DESCRIPTOR_FAILURE:
    printInfo("descriptor during new directives");
    rc_t = OK;
    break;
  case DEAD_PROCESS:
    printInfo("one or more workers are dead, new one/s is/are spawned");
    rc_t = OK;
    break;
  case START_NEW_MANAGER:
    /* printInfo("manager created"); */
    rc_t = OK;
    break;
  default:
    fprintf(stderr, "rc magico %d\n", errorCode);
    printError("unknown error");
    rc_t = HARAKIRI;
    break;
  }

  return rc_t;
}
