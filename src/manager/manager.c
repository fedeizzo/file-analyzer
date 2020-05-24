#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../config/config.h"
#include "../list/list.h"
#include "../manager/manager.h"
#include "../table/table.h"
#include "../worker/worker.h"
#include "../wrapping/wrapping.h"

// TODO see const arguments to function

void stopThisShitPrint(void *data) {
  Work w = (Work)data;
  printf("NAME: %s\n", w->tablePointer->name);
  printf("START: %d\n", w->bufferStart);
  printf("END: %d\n", w->bufferEnd);
}

void printWorker(void *data) {
  Worker w = (Worker)data;
  printf("il puntatore %p\n", w->doing);
  /* stopThisShitPrint(w->doing); */
  printf("work amoutn: %d\n", w->workAmount);
}

Worker newWorker() {
  int rc_al = SUCCESS;
  int rc_al2 = SUCCESS;
  int rc_al3 = SUCCESS;
  Worker worker = malloc(sizeof(struct structWorker));
  rc_al = checkAllocationError(worker);
  if (rc_al == SUCCESS) {
    worker->table = calloc(NCHAR_TABLE, sizeof(unsigned long long));
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

  if (worker->pipe != NULL) {
    closeDescriptor(worker->pipe[0]);
    closeDescriptor(worker->pipe[1]);
  }
  // fprintf(stderr, "prima free pipe\n");
  free(worker->pipe);
  // fprintf(stderr, "dopo free pipe\n");
  free(worker->table);
  // fprintf(stderr, "dopo free table\n");
  free(worker);
  // fprintf(stderr, "dopo free worker\n");
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
  int rc_al = SUCCESS;
  int rc_al2 = SUCCESS;
  int rc_al3 = SUCCESS;
  Directive directive = malloc(sizeof(struct DirectivesStruct));
  rc_al = checkAllocationError(directive);

  if (rc_al == SUCCESS) {
    directive->paths = newList();
    if (directive->paths == NULL)
      rc_al2 = MALLOC_FAILURE;
    directive->currentWorkers = 4;
    directive->directiveStatus = START_NEW_MANAGER;
  }

  if (rc_al < 0 || rc_al2 < 0 || rc_al3 < 0)
    directive = NULL;

  return directive;
}

void destroyDirective(Directive directive) {
  destroyList(directive->paths, free);
  // fprintf(stderr, "FREEEE DIRECTIVE\n");
  free(directive);
  // fprintf(stderr, "dopo free directive\n");
}

/**
 * Initializes manager
 *
 * args:
 *    List workers: workers list
 *    const int nWorkers: number of workers
 *    List tables: tables list
 *    List todo: todo works list
 *
 * returns
 *    0 in case of success, negative number otherwise
 */
int initManager(List workers, const int nWorkers, List tables, List todo);

/**
 * Deinitializes manger
 *
 * args:
 *    List workers: workers list
 *    List tables: tables list
 *    List todo: todo works list
 *    List paths: list of paths
 *
 * returns
 *    0 in case of success, negative number otherwise
 */
void deinitManager(List workers, List tables, List todo);

/**
 * Changes workers amount under manager control
 *
 * args:
 *    List workers: workers list
 *    const int currentWorkers: number of current workers
 *    const int newWorkers: number of new workers
 *    List tables: tables list
 *    List todo: todo works list
 *
 * returns
 *    0 in case of success, negative number otherwise
 */
int changeWorkersAmount(List workers, const int currentWorkers,
                        const int newWorkers, List tables, List todo);

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
 *
 * returns
 *    0 in case of success, negative number otherwise
 */
int removeWorkers(List workers, int amount, List tables, List todo);

/**
 * Executes the manger manager
 *
 * args:
 *    List workers: workers list
 *    List tables: tables list
 *    List todo: todo works list
 *
 * returns
 *    0 in case of success, negative number otherwise
 */
int executeWork(List workers, List tables, List todo, int *summaryFlag);

/**
 * Assigns work to worker
 *
 * args:
 *    Worker worker: the worker that recives the work
 *    Work work: the work that is assigned
 *    List todo: todo works list
 *
 * returns
 *    0 in case of success, negative number otherwise
 */
int assignWork(Worker worker, Work work, List todo);

/**
 * Gets chars from workers
 *
 * args:
 *    List workers: workers list
 *    List tables: tables list
 *    List todo: todo works list
 *
 * returns
 *    0 in case of success, negative number otherwise
 */
int getWorkerWork(Worker w, List tables, List todo, int *summaryFlag);

/**
 * Checks if a work is done. If so sends summary is called
 *
 * args:
 *    int *summaryFlag: flag that indicate if a work is done
 *
 * returns:
 *    0 in case there are new work in done list, -1 otherwise
 */
int checkUpdate(int *summaryFlag);

/**
 * Updates manger table
 *
 * args:
 *    int *table: the table associated with a file
 *    int *workerTable: the worker table for the udpate operation
 */
void updateTable(unsigned long long *table, unsigned long long *workerTable);

/**
 * Ends a worker's work
 *
 * args:
 *    Worker worker: the worker which has ended work
 *    List tables: tables list
 *    int typeEnding: the ending type
 *    List todo: todo works list
 *
 * returns
 *    0 in case of success, negative number otherwise
 */
int endWork(Worker worker, List tables, int typeEnding, List todo,
            int *summaryFlag);

/**
 * Reads all infos from pipe
 *
 * args:
 *    void *ptr: pointer to pthrad struct
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
 *    List tables: tables list
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

// TODO... REMOVE PRINT FUNCTION FOR DEBUGGING
void toStringTable(void *data) {
  Table table = (Table)data;
  printf("name: %s\n", table->name);
  printf("work associated: %d\n", table->workAssociated);
}

void printFigo(void *data) {
  char *s = (char *)data;
  // fprintf(stderr, "contengo percorso: %s\n", s);
}

// TODO write docs
int clearWorkersWork(List workers, List todo, List tables);
int remoduleWorks(List todo, List workers, List tables);

int main(int argc, char *argv[]) {
  signal(SIGCHLD, SIG_IGN);
  pthread_t directives, work;
  int iret1, iret2;
  List workers = newList();
  List tables = newList();
  List todo = newList();

  int currentWorkers = 4;
  int newNWorker;

  int rc_work = initManager(workers, currentWorkers, tables, todo);

  sharedResources_t sharedResourses;
  sharedResourses.directive = newDirective();
  pthread_mutex_init(&sharedResourses.mutex, NULL);
  sharedResourses.todo = todo;
  sharedResourses.workers = workers;
  sharedResourses.tables = tables;
  sharedResourses.summaryFlag = 0;

  if (rc_work == SUCCESS) {
    iret1 = pthread_create(&directives, NULL, readDirectives,
                           (void *)&sharedResourses);
    iret2 = pthread_create(&work, NULL, workLoop, (void *)&sharedResourses);

    pthread_join(directives, NULL);
    pthread_join(work, NULL);
  }

  deinitManager(workers, tables, todo);
  destroyDirective(sharedResourses.directive);
  return rc_work;
}

void print(void *data) {
  Work w = (Work)data;
  printf("path: %s\n", w->tablePointer->name);
  printf("start: %d\n", w->bufferStart);
  printf("end: %d\n", w->bufferEnd);
}

void *workLoop(void *ptr) {
  sharedResources_t *sharedRes = (sharedResources_t *)(ptr);

  int rc_work = SUCCESS;
  int rc_nd = SUCCESS;
  int rc_wc = SUCCESS;
  int directives;
  int workDone = 0;

  while (rc_work == SUCCESS) {
    pthread_mutex_lock(&(sharedRes->mutex));
    directives = sharedRes->directive->directiveStatus;
    pthread_mutex_unlock(&(sharedRes->mutex));
    while (directives == START_NEW_MANAGER) {
      pthread_mutex_lock(&(sharedRes->mutex));
      directives = sharedRes->directive->directiveStatus;
      pthread_mutex_unlock(&(sharedRes->mutex));
      usleep(5000);
    }
    pthread_mutex_lock(&(sharedRes->mutex));
    if (checkUpdate(&sharedRes->summaryFlag) == SUMMARY) {
      sendSummary(sharedRes->tables);
    }
    pthread_mutex_unlock(&(sharedRes->mutex));
    pthread_mutex_lock(&(sharedRes->mutex));
    if (directives == STOP_MANAGER) {
      clearWorkersWork(sharedRes->workers, sharedRes->todo, sharedRes->tables);
      destroyList(sharedRes->tables, destroyTable);
      destroyList(sharedRes->todo, destroyWork);

      sharedRes->tables = newList();
      sharedRes->todo = newList();
      if (sharedRes->tables == NULL || sharedRes->todo == NULL)
        rc_work = MALLOC_FAILURE;
      sharedRes->directive->directiveStatus = SUMMARY;
    } else if (directives == NEW_DIRECTIVES ||
               sharedRes->directive->paths->size > 0) {

      if (sharedRes->directive->currentWorkers !=
          sharedRes->directive->newNWorker) {
        rc_wc = changeWorkersAmount(sharedRes->workers,
                                    sharedRes->directive->currentWorkers,
                                    sharedRes->directive->newNWorker,
                                    sharedRes->tables, sharedRes->todo);
        sharedRes->directive->currentWorkers = sharedRes->directive->newNWorker;
        remoduleWorks(sharedRes->todo, sharedRes->workers, sharedRes->tables);
        sharedRes->directive->directiveStatus = SUMMARY;
      }

      if (sharedRes->directive->paths->size > 0) {
        // TODO add control
        char *path = front(sharedRes->directive->paths);
        ////fprintf(stderr, "sto per aggiungere le direttive al percorso %s
        /// con
        /// size %d ---- %d\n", path,sharedRes->directive->paths->size,
        /// getpid());
        pop(sharedRes->directive->paths);
        ////fprintf(stderr, "->Ho size %d ---- %d\n",
        /// sharedRes->directive->paths->size, getpid());
        rc_nd = addDirectives(sharedRes->tables, sharedRes->todo, path,
                              sharedRes->directive->currentWorkers);
        // fprintf(stderr, "FREE PATH IN WORKLOOP\n");
        free(path);
        // fprintf(stderr, "dopo free path\n");
      } else {
        // //fprintf(stderr, " !!! asdkamsudbaisdniasdbaisd %d !!! \n",
        // getpid());
        // printList(sharedRes->directive->paths, printFigo);
      }

      if (rc_nd < SUCCESS && rc_wc < SUCCESS) {
        int r1 = errorHandler(rc_nd);
        int r2 = errorHandler(rc_wc);
        if (r1 != SUCCESS || r2 != SUCCESS)
          rc_work = HARAKIRI;
      } else if (rc_nd < SUCCESS) {
        rc_work = errorHandler(rc_nd);
      } else if (rc_wc < SUCCESS) {
        rc_work = errorHandler(rc_wc);
      }
    } else if (directives < SUCCESS) {
      rc_work = errorHandler(directives);
    }
    pthread_mutex_unlock(&(sharedRes->mutex));
    if (rc_work == SUCCESS) {
      /* printf("potrei rompermi qui\n"); */
      pthread_mutex_lock(&(sharedRes->mutex));
      rc_work = executeWork(sharedRes->workers, sharedRes->tables,
                            sharedRes->todo, &sharedRes->summaryFlag);
      pthread_mutex_unlock(&(sharedRes->mutex));
      /* printf("e invece no\n"); */
      if (rc_work < SUCCESS)
        rc_work = errorHandler(rc_work);
    }
    usleep(1);
  }
  // fprintf(stderr, "MUOIO CON rc_work pari a %d\n", rc_work);
  kill(getpid(), SIGKILL);
}

int initManager(List workers, const int nWorkers, List tables, List todo) {
  int rc_t = SUCCESS;

  if (workers != NULL && tables != NULL && todo != NULL) {
    int i = 0;
    for (i = 0; i < nWorkers && rc_t == SUCCESS; i++) {
      rc_t = addWorkers(workers, 1);
    }
  } else {
    rc_t = INIT_FAILURE;
  }
  return rc_t;
}

void deinitManager(List workers, List tables, List todo) {
  destroyList(workers, destroyWorker);
  /* printf("SONO IN DEINIT MANAGER\n"); */
  destroyList(tables, destroyTable);
  destroyList(todo, destroyWork);
}

int changeWorkersAmount(List workers, const int currentWorkers,
                        const int newWorkers, List tables, List todo) {
  int rc_t;
  int delta;
  if (currentWorkers > 0)
    delta = newWorkers - currentWorkers;
  else
    delta = newWorkers;

  if (delta > 0) {
    rc_t = addWorkers(workers, delta);
  } else {
    rc_t = removeWorkers(workers, -delta, tables, todo);
  }

  return rc_t;
}

int addWorkers(List workers, int amount) {
  int rc_t = SUCCESS;
  int rc_en = SUCCESS;
  int i = 0;
  for (i = 0; i < amount && rc_t == SUCCESS; i++) {
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
          // fprintf(stderr, "MUOIO CREANDO I WORKER\n");
          kill(getpid(), SIGKILL);
        }
      }
    }
  }

  return rc_t;
}

int removeWorkers(List workers, int amount, List tables, List todo) {
  int rc_t = SUCCESS;

  while (amount != 0 && workers->size != 0) {
    Worker w = front(workers);
    if (w != NULL) {
      int rc_po = pop(workers);
      endWork(w, tables, BAD_ENDING, todo, NULL);
      kill(w->pid, SIGKILL);
      // fprintf(stderr, "SONO IN REMOVE WORKERS\n");
      destroyWorker(w);
      // fprintf(stderr, "ESCO DA REMOVE WORKERS\n");
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
  int rc_t = SUCCESS;
  int returnCode = kill(w->pid, 0);
  if (returnCode != 0) {
    rc_t = DEAD_PROCESS;
  }
  return rc_t;
}

int executeWork(List workers, List tables, List todo, int *summaryFlag) {
  int rc_t = SUCCESS;
  int rc_po = SUCCESS;
  int rc_pu = SUCCESS;
  int rc_ww = SUCCESS;
  int workerSize = workers->size;
  int isWorkerAlive = SUCCESS;
  List newWorkers = newList();

  int i = 0;
  for (i = 0; i < workerSize; i++) {
    Worker w;
    w = front(workers);
    if (w != NULL) {
      // fprintf(stderr, "entro in isAlive\n");
      isWorkerAlive = isAlive(w);
      // fprintf(stderr, "Sono uscito da isAlive con %d\n", isWorkerAlive);
      rc_po = pop(workers);
      // fprintf(stderr, "HO APPENA FATTO POP\n");
      if (isWorkerAlive == SUCCESS) {
        if (rc_po == -1)
          rc_t = NEW_WORKER_FAILURE;
        else {
          if (w->doing != NULL) {
            // fprintf(stderr, "Sono prima di getWorkerWork\n");
            rc_ww = getWorkerWork(w, tables, todo, summaryFlag);
            // fprintf(stderr, "Sono uscito da getWorkerWork\n");
            if (rc_ww < SUCCESS)
              rc_t = WORK_FAILURE;
          } else {
            if (todo->size != 0) {
              /* printf("size: %d\n", todo->size); */
              Work work = front(todo);
              if (work != NULL) {
                // fprintf(stderr, "Sono prima di assignWork\n");
                rc_t = assignWork(w, work, todo);
                // fprintf(stderr, "Sono uscito da assignWork\n");
              } else {
                rc_t = ASSIGNWORK_FAILURE;
              }
            }
          }
        }
        rc_pu = enqueue(workers, w);
        // fprintf(stderr, "HO APPENA FATTO enqueue\n");
        if (rc_pu == -1)
          rc_t = NEW_WORKER_FAILURE;
      } else {
        // TODO find better way to do this
        if (w->doing != NULL) {
          rc_pu = push(todo, w->doing);
          // fprintf(stderr, "HO APPENA FATTO psh\n");
          if (rc_pu < SUCCESS)
            rc_t = MALLOC_FAILURE;
          else {
            destroyWorker(w);
            // fprintf(stderr, "ramo else dell'executeWork\n");
            addWorkers(newWorkers, 1);
            // fprintf(stderr, "dopo addWorkers del ramo else dell'execute
            // work\n");
            rc_t = DEAD_PROCESS;
          }
        } else {
          destroyWorker(w);
          // fprintf(stderr, "ramo else pt 2 dell'executeWork\n");
          addWorkers(newWorkers, 1);
          rc_t = DEAD_PROCESS;
        }
      }
    }
  }
  // fprintf(stderr, "Prima del blocco concat\n");
  if (newWorkers->size > 0 && workers->size > 0) {
    // fprintf(stderr, "prima concat\n");
    int rc_cat = concat(workers, newWorkers);
    // fprintf(stderr, "dopo concat\n");
    if (rc_cat < SUCCESS)
      rc_t = NEW_WORKER_FAILURE;
  } else if (workers->size == 0 && newWorkers->size > 0) {
    // fprintf(stderr, "prima swap\n");
    int rc_cat = swap(workers, newWorkers);
    // fprintf(stderr, "dopo swap\n");
    if (rc_cat < SUCCESS)
      rc_t = NEW_DIRECTIVES_FAILURE;
  }
  // fprintf(stderr, "Dopo il blocco concat\n");
  // fprintf(stderr, "FREEE NEWWORKER\n IN EXECUTE WORK\n");
  free(newWorkers);
  // fprintf(stderr, "dopo free nWorkers\n");

  return rc_t;
}

int assignWork(Worker worker, Work work, List todo) {
  int rc_t = SUCCESS;
  int rc_po = pop(todo);
  /* printf("return code di pop: %d\n", rc_po); */
  if (rc_po != -1) {
    worker->doing = work;
    worker->workAmount = work->bufferEnd - work->bufferStart + 1;
    int *pipe = worker->pipe;
    char *path = malloc(PATH_MAX * sizeof(char));
    int rc_al = checkAllocationError(path);
    char *bufferStart = malloc(PATH_MAX * sizeof(char));
    int rc_al2 = checkAllocationError(bufferStart);
    char *bufferEnd = malloc(PATH_MAX * sizeof(char));
    int rc_al3 = checkAllocationError(bufferEnd);

    if (rc_al < SUCCESS || rc_al2 < SUCCESS || rc_al3 < SUCCESS)
      rc_t = MALLOC_FAILURE;
    else {
      int rc_ca = sprintf(path, "%s", work->tablePointer->name);
      int rc_ca2 = sprintf(bufferStart, "%d", work->bufferStart);
      int rc_ca3 = sprintf(bufferEnd, "%d", work->bufferEnd);
      /* fprintf(stderr, "ASDASDASDASDASD %s %s %s\n", path, bufferStart, */
      /*         bufferEnd); */
      if (rc_ca == SUCCESS || rc_al2 == SUCCESS || rc_al3 == SUCCESS) {
        int rc_wr = writeDescriptor(pipe[WRITE_CHANNEL], path);
        int rc_wr2 = writeDescriptor(pipe[WRITE_CHANNEL], bufferStart);
        int rc_wr3 = writeDescriptor(pipe[WRITE_CHANNEL], bufferEnd);

        if (rc_wr < SUCCESS || rc_wr2 < SUCCESS || rc_wr3 < SUCCESS)
          rc_t = SEND_FAILURE;
      } else
        rc_t = SEND_FAILURE;
      // fprintf(stderr, "prima free path\n");
      free(path);
      // fprintf(stderr, "dopo free pipe\n");
      free(bufferStart);
      // fprintf(stderr, "dopo free bufferStart\n");
      free(bufferEnd);
      // fprintf(stderr, "dopo free bufferEnd\n");
    }
  } else
    rc_t = ASSIGNWORK_MEMORY_FAILURE;

  // printf("size: %d\n", todo->size);
  return rc_t;
}

int getWorkerWork(Worker w, List tables, List todo, int *summaryFlag) {
  int rc_t = SUCCESS;
  int readFromWorker = w->pipe[READ_CHANNEL];
  int bytesSent = w->bytesSent;
  char *charSent = malloc(w->workAmount * sizeof(char));
  int rc_al = checkAllocationError(charSent);
  if (rc_al < SUCCESS)
    rc_t = MALLOC_FAILURE;
  if (rc_t == SUCCESS) {
    if (bytesSent >= w->workAmount) {
      int rc_rd = read(readFromWorker, charSent, 5);
      if (rc_rd <= 0) {
        rc_t = READ_FAILURE;
        // endWork(w, tables, BAD_ENDING, todo, NULL);
      } else {
        charSent[rc_rd] = '\0';
        // printf("la parola di controllo: %s\n", charSent);
        if (strncmp(charSent, "done", 4) == 0) {
          endWork(w, tables, GOOD_ENDING, todo, summaryFlag);
        } else {
          endWork(w, tables, BAD_ENDING, todo, NULL);
          rc_t = WORK_FAILURE;
        }
      }
    } else {
      int rc_rd = read(readFromWorker, charSent, w->workAmount - w->bytesSent);
      if (rc_rd <= 0)
        rc_t = READ_FAILURE;
      else {
        int i = 0;
        for (i = 0; i < rc_rd; i++) {
          int charCode = charSent[i];
          if (charCode < 128 && charCode > 0) {
            if (w->table[charCode] < ULLONG_MAX) {
              w->table[charCode] += 1;
            } else {
              rc_t = CHARACTER_OVERFLOW;
            }
          } else {
            if (w->table[128] < ULLONG_MAX) {
              w->table[128] += 1;
            } else {
              rc_t = CHARACTER_OVERFLOW;
            }
          }
          w->bytesSent++;
        }
      }
    }
    // fprintf(stderr, "CHARSENT FREE GET WORKER WORK\n");
    free(charSent);
    // fprintf(stderr, "DOPO CHARSENT FREE GET WORKER WORK\n");
  }
  return rc_t;
}

int checkUpdate(int *summaryFlag) {
  int rc_t = -1;
  if (*summaryFlag == 1) {
    *summaryFlag = 0;
    rc_t = SUMMARY;
  }
  return rc_t;
}

void updateTable(unsigned long long *table, unsigned long long *workerTable) {
  int i = 0;
  for (i = 0; i < NCHAR_TABLE; i++)
    table[i] += workerTable[i];
}

int endWork(Worker worker, List tables, int typeEnding, List todo,
            int *summaryFlag) {
  int rc_t = SUCCESS;
  int rc_ca = SUCCESS;
  Work work = worker->doing;
  unsigned long long *workerTable = worker->table;
  // printf("Entro nell'end work con %d\n", typeEnding);

  if (typeEnding == GOOD_ENDING) {
    updateTable(work->tablePointer->table, workerTable);
    work->tablePointer->workAssociated--;
    *summaryFlag = 1;

    destroyWork(work);
  } else {
    if (work != NULL) {
      int rc_pu = push(todo, work);
      if (rc_pu == -1)
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

int sendStopMessage(Worker worker) {
  int rc_t = SUCCESS;

  int *pipe = worker->pipe;
  int rc_wr = writeDescriptor(pipe[WRITE_CHANNEL], "stop");
  int rc_wr2 = writeDescriptor(pipe[WRITE_CHANNEL], "stop");
  int rc_wr3 = writeDescriptor(pipe[WRITE_CHANNEL], "stop");

  if (rc_wr < SUCCESS || rc_wr2 < SUCCESS || rc_wr3 < SUCCESS)
    rc_t = SEND_FAILURE;

  return rc_t;
}

int clearWorkersWork(List workers, List todo, List tables) {
  int rc_t = SUCCESS;
  int rc_sd = SUCCESS;
  int rc_pu = SUCCESS;

  Node list = workers->head;
  while (list != NULL && rc_t != MALLOC_FAILURE) {
    Worker worker = list->data;
    if (worker->doing != NULL) {
      int rc_sd = sendStopMessage(worker);

      if (rc_sd == SUCCESS) {
        char *charSent = malloc(worker->workAmount * sizeof(char));
        int rc_al = checkAllocationError(charSent);
        if (rc_al == SUCCESS) {
          while (worker->bytesSent < worker->workAmount &&
                 isAlive(worker) == 0) {
            int rc_rd = read(worker->pipe[READ_CHANNEL], charSent,
                             worker->workAmount - worker->bytesSent);
            if (rc_rd > 0)
              worker->bytesSent += rc_rd;
            /* printf("ho letto %d\n", rc_rd); */
            /* printf("ho letto %d su %d\n", worker->bytesSent, */
            /*        worker->workAmount); */
          }
          int rc_rd = -1;
          while (rc_rd == -1 && isAlive(worker) == SUCCESS) {
            rc_rd = read(worker->pipe[READ_CHANNEL], charSent, 5);
            /* printf("ho letto %s\n", charSent); */
            if (rc_rd == 5) {
              if (strncmp(charSent, "done", 4) == 0) {
                /* printf("sto per entrare nel while strano\n"); */
                endWork(worker, tables, BAD_ENDING, todo, NULL);
              }
            }
          }
        } else {
          rc_t = MALLOC_FAILURE;
        }
      } else
        rc_t = SEND_FAILURE;
    }

    list = list->next;
  }

  return rc_t;
}

int remoduleWorks(List todo, List workers, List tables) {
  int rc_t = SUCCESS;

  /* printf("prima\n----------------------------------------------\n"); */
  /* printList(todo, print); */

  rc_t = clearWorkersWork(workers, todo, tables);

  /* printf("return code: %d\n", rc_t); */
  /* printf("dopo\n----------------------------------------------\n"); */
  /* printList(todo, print); */
  /* printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"); */
  /* printList(tables, toStringTable); */

  int todoSize = todo->size;
  int nWorkers = workers->size;

  int i, j;
  int rc_pu = SUCCESS;
  int rc_po = SUCCESS;
  //! TODO... Add t->workAssociated
  for (i = 0; i < todoSize && rc_t == SUCCESS; i++) {
    Work work = front(todo);
    if (work != NULL) {
      rc_po = pop(todo);
      if (rc_po == SUCCESS) {
        work->tablePointer->workAssociated--;
        int workDimension = work->bufferEnd - work->bufferStart + 1;
        if (workDimension >= nWorkers && nWorkers > 0) {
          int step = (int)workDimension / nWorkers;
          int remainder = workDimension % nWorkers;
          int rc_pu2 = SUCCESS;
          for (j = 0; j < nWorkers - 1 && rc_pu2 == SUCCESS; j++) {

            Work w = newWork(work->tablePointer, step * j + work->bufferStart,
                             step * (j + 1) + work->bufferStart - 1);
            rc_pu2 = enqueue(todo, w);
          }
          if (rc_pu2 == SUCCESS) {
            Work w = newWork(
                work->tablePointer, step * (nWorkers - 1) + work->bufferStart,
                step * nWorkers + remainder + work->bufferStart - 1);
            rc_pu2 = enqueue(todo, w);
          } else {
            rc_t = NEW_DIRECTIVES_FAILURE;
          }
          work->tablePointer->workAssociated += nWorkers;
        } else {
          int rc_pu2 = enqueue(todo, work);
          if (rc_pu2 < SUCCESS)
            rc_t = NEW_DIRECTIVES_FAILURE;
          else
            work->tablePointer->workAssociated++;
        }
      } else
        rc_t = 0;
    }
  }
  /* printf("dopo dopo\n----------------------------------------------\n"); */
  /* printList(todo, print); */
  /* printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"); */
  /* printList(tables, toStringTable); */
  /* printf("dopo finale\n----------------------------------------------\n"); */

  return rc_t;
}

void *readDirectives(void *ptr) {
  sharedResources_t *sharedRes = (sharedResources_t *)(ptr);
  int rc_t = SUCCESS;
  char readBuffer[2] = "a";
  char nWorker[PATH_MAX];
  int counter = 0;
  int castPlaceHolder = 0;
  int stopFlag = 0;

  while (rc_t == SUCCESS) {
    // TODO check error allcoation
    char *newPath = malloc(PATH_MAX * sizeof(char));
    /* int rc_al = checkAllocationError(newPath); */
    /* if (rc_al < SUCCESS) { */
    /*   rc_t = MALLOC_FAILURE; */
    /* } */
    counter = 0;
    do {
      int rc = readChar(READ_CHANNEL, readBuffer);
      newPath[counter++] = readBuffer[0];
    } while (readBuffer[0] != '\0' && readBuffer[0] != '\n');
    newPath[counter] = '\0';
    if (newPath[strlen(newPath) - 1] == '\n') {
      newPath[strlen(newPath) - 1] = '\0';
    }
    if (strncmp(newPath, "stop", 4) == 0)
      stopFlag = 1;

    counter = 0;
    do {
      int rc = readChar(READ_CHANNEL, readBuffer);
      nWorker[counter++] = readBuffer[0];
    } while (readBuffer[0] != '\0' && readBuffer[0] != '\n');
    nWorker[counter] = '\0';
    if (strncmp(nWorker, "stop", 4) == 0 && stopFlag == 1)
      stopFlag = 1;
    else
      stopFlag = 0;

    // TODO... debug only
    fprintf(stderr, "Path %s, nWorker %s pid %d\n", newPath, nWorker, getpid());
    // usleep(10000);

    pthread_mutex_lock(&(sharedRes->mutex));
    int rc_sc = sscanf(nWorker, "%d", &castPlaceHolder);
    if (stopFlag != 1 &&
        (rc_sc == 0 || (castPlaceHolder == 9 && strcmp(nWorker, "9") == 0))) {
      rc_t = CAST_FAILURE;
    } else
      sharedRes->directive->newNWorker = castPlaceHolder;
    ////fprintf(stderr, "prima ---------------------------------\n");
    // printList(sharedRes->directive->paths, printFigo);
    ////fprintf(stderr, "finito ---------------------------------\n");
    if (stopFlag != 1)
      enqueue(sharedRes->directive->paths, newPath);
    ////fprintf(stderr, "dopo ---------------------------------\n");
    // printList(sharedRes->directive->paths, printFigo);
    ////fprintf(stderr, "finito ---------------------------------\n");

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
    } else if (rc_t == SUCCESS) {
      if (stopFlag == 1) {
        sharedRes->directive->directiveStatus = STOP_MANAGER;
        stopFlag = 0;
      } else
        sharedRes->directive->directiveStatus = NEW_DIRECTIVES;
    }
    pthread_mutex_unlock(&(sharedRes->mutex));
    /* free(newPath); */
    usleep(5);
  }
  // fprintf(stderr, "MANAGER: MUOIO CON rc_t pari a %d\n", rc_t);
  kill(getpid(), SIGKILL);
  // return rc_t;
}

int addDirectives(List tables, List todo, const char *path, const int nWorker) {
  int rc_t = SUCCESS;
  Table t = newTable(path);
  int rc_al = checkAllocationError(t);
  List todoTmp = newList();
  int rc_al2 = checkAllocationError(todoTmp);
  ////fprintf(stderr, "path in addDirectives %s\n", path);
  if (rc_al == -1 || rc_al2 == -1)
    rc_t = TABLE_FAILURE;
  else {
    int rc_pu = push(tables, t);
    if (rc_pu == 0) {
      int fd = openFile(path, O_RDONLY);
      unsigned long long fileDimension = moveCursorFile(fd, 0, SEEK_END);
      //printf("file dimension %lli\n", fileDimension);
      if (fileDimension > 0 && nWorker > 0 && fileDimension < nWorker) {
        Work w = newWork(t, 0, fileDimension - 1);
        // TODO... check error code
        push(todoTmp, w);
        // TODO... check if this causes any bug!!!
        t->workAssociated = 1;
      } else if (fileDimension > 0 && nWorker > 0) {
        unsigned long long step = (unsigned long long)fileDimension / nWorker;
        unsigned long long remainder = fileDimension % nWorker;
        int rc_pu2 = SUCCESS;

          int i = 0;
          for (i = 0; i < nWorker - 1 && rc_pu2 == SUCCESS; i++) {
            Work w = newWork(t, step * i, step * (i + 1) - 1);
            rc_pu2 = push(todoTmp, w);
          }

          if (rc_pu2 == SUCCESS) {
            // TODO per considerare anche EOF mettere solo -1
            Work w = newWork(t, step * (nWorker - 1),
                             step * nWorker + remainder - 1);
            rc_pu2 = push(todoTmp, w);
          } else {
            rc_t = NEW_DIRECTIVES_FAILURE;
          }
          t->workAssociated = nWorker;
        } else if (fileDimension > 0 && nWorker == 0) {
          Work w = newWork(t, 0, fileDimension - 1);
          // TODO... check error code
          push(todoTmp, w);
          // TODO... check if this causes any bug!!!
          t->workAssociated = 1;
        } else if (fileDimension == 0) {
          //! da togliere senno' crasha analyzer
          // printf("entro nell'else\n");
          // TODO... check if this causes any bug!!!
          // pop(tables);
          // TODO... check error code
          Work w = newWork(t, 0, -1);
          push(todoTmp, w);
          t->workAssociated = 1;
        }
        //! da togliere senno' crasha analyzer
        // printList(todoTmp, stopThisShitPrint);

        int rc_cl = SUCCESS;
        if (fd != -1)
          rc_cl = closeDescriptor(fd);

        if (fd == -1 || rc_cl == -1)
          rc_t = DESCRIPTOR_FAILURE;
      } else {
        rc_t = NEW_DIRECTIVES_FAILURE;
      }
    }
    ////fprintf(stderr, "Stampo in add directives\n");
    // printList(tables, toStringTable);
  }

  if (rc_t == SUCCESS) {
    if (todoTmp->size > 0 && todo->size > 0) {
      int rc_cat = concat(todo, todoTmp);
      if (rc_cat < SUCCESS)
        rc_t = NEW_DIRECTIVES_FAILURE;
    } else if (todoTmp->size > 0 && todo->size == 0) {
      int rc_cat = swap(todo, todoTmp);
      if (rc_cat < SUCCESS)
        rc_t = NEW_DIRECTIVES_FAILURE;
    }
  }
  // fprintf(stderr, "FREEEEEE TODO TMP\n");
  free(todoTmp);
  // fprintf(stderr, "dopo free todo\n");
  /* printf("prima dell'add directives\n"); */
  /* printList(todo, print); */
  /* printList(tables, toStringTable); */
  /* printf("dopo dell'add directives\n"); */

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
  /* printf("ATTENZIONE: entro nel summary\n"); */
  int rc_t = SUCCESS;
  int rc_po = SUCCESS;
  int rc_pu = SUCCESS;
  int tablesSize = tables->size;
  // TODO remove print
  // printList(tables, toStringTable);
  int i = 0;

  for (i = 0; i < tablesSize; i++) {
    Table t;
    t = front(tables);
    rc_po = pop(tables);
    if (rc_po == -1 || rc_pu == -1)
      rc_t = SUMMARY_FAILURE;
    if (t != NULL) {
      int j = 0;
      // TODO remove this acc
      unsigned long long acc = 0;
      writeDescriptor(WRITE_CHANNEL, t->name);
      // writeDescriptor(WRITE_CHANNEL, "\n");
      ////fprintf(stderr, "-----------------MANAGER INVIO %s\n", t->name);
      for (j = 0; j < NCHAR_TABLE; j++) {
        // TODO choose PATH_MAX
        char msg[PATH_MAX];
        int rc_sp;
        rc_sp = sprintf(msg, "%lld", t->table[j]);
        acc += t->table[j];
        if (rc_sp == -1)
          rc_t = CAST_FAILURE;
        else {
          int rc_wr = writeDescriptor(WRITE_CHANNEL, msg);
          if (rc_wr == -1)
            rc_t = SUMMARY_FAILURE;
        }
      }
      if (t->workAssociated == 0) {
        destroyTable(t);
        writeDescriptor(WRITE_CHANNEL, "done");
      } else {
        enqueue(tables, t);
        writeDescriptor(WRITE_CHANNEL, "undo");
      }

      // TODO remove this acc
      fprintf(stderr, "acc: %llu\n", acc);
    }
  }

  /* printf("ATTENZIONE: esco dal summary\n"); */
  return rc_t;
}

int errorHandler(int errorCode) {
  int rc_t = SUCCESS;

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
    rc_t = SUCCESS;
    break;
  case READ_FAILURE:
    /* printInfo("reading from worker"); */
    rc_t = SUCCESS;
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
    rc_t = SUCCESS;
    break;
  case PIPE_FAILURE:
    printInfo("no communication pipe between worker and manager");
    rc_t = SUCCESS;
    break;
  case SUMMARY_FAILURE:
    printError("no memory for new tables or no communication with parent");
    rc_t = HARAKIRI;
    break;
  case REMOVE_WORK_FAILURE:
    printInfo("no worker to remove");
    rc_t = SUCCESS;
    break;
  case INIT_FAILURE:
    printError("no memory to start manager process");
    rc_t = HARAKIRI;
    break;
  case ASSIGNWORK_FAILURE:
    printInfo("no work to assign");
    rc_t = SUCCESS;
    break;
  case SEND_FAILURE:
    printInfo("not able to send work to worker");
    rc_t = SUCCESS;
    break;
  case ASSIGNWORK_MEMORY_FAILURE:
    printError("no memory to assign work");
    rc_t = HARAKIRI;
    break;
  case DESCRIPTOR_FAILURE:
    printInfo("descriptor during new directives");
    rc_t = SUCCESS;
    break;
  case DEAD_PROCESS:
    printInfo("one or more workers are dead, new one/s is/are spawned");
    rc_t = SUCCESS;
    break;
  case START_NEW_MANAGER:
    /* printInfo("manager created"); */
    rc_t = SUCCESS;
    break;
  case CHARACTER_OVERFLOW:
    printInfo("Preventing characters overflow, the statistics could not be "
              "reliable");
    rc_t = SUCCESS;
    break;
  default:
    // fprintf(stderr, "rc magico %d\n", errorCode);
    printError("unknown error");
    rc_t = HARAKIRI;
    break;
  }

  return rc_t;
}
