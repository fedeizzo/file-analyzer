#include <fcntl.h>
#include <limits.h>
#include <linux/limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../config/config.h"
#include "../list/list.h"
#include "../tui/tui.h"
#include "../wrapping/wrapping.h"
#include "./reporter.h"

const char *helpMsgV2 =
    "Usage: tui [OPTIONS] [FILES] [FOLDERS]\n\n"
    "OPTIONS:\n"
    "\t-h, --help        : print this message\n"
    "\t-n, --number     : specify the managers amount\n"
    "\t-m, --mumber      : specify the workers amount\n"
    "\t-f,,--normal-mode : normal mode without fancy graphic\n";

UserInput newUserInput() {
  UserInput ui = malloc(sizeof(struct UserInputStr));
  int rc_al = checkAllocationError(ui);
  int rc_al2 = SUCCESS;
  int rc_al3 = SUCCESS;
  int rc_al4 = SUCCESS;

  if (rc_al == SUCCESS) {
    ui->paths = newList();
    ui->results = newList();
    ui->directories = newList();
    ui->files = newList();
    ui->tree = malloc(PATH_MAX * sizeof(char));
    ui->table = calloc(NCHAR_TABLE, sizeof(unsigned long long));
    // TODO Check allocation
    ui->tree[0] = '\0';
    if (ui->paths == NULL)
      rc_al2 = -1;
    if (ui->results == NULL)
      rc_al3 = -1;
    rc_al4 = checkAllocationError(ui->paths);
    ui->managers = 3;
    ui->workers = 4;
    ui->toggledChanged = 0;
  }

  if (rc_al < SUCCESS || rc_al2 < SUCCESS || rc_al3 < SUCCESS ||
      rc_al4 < SUCCESS)
    ui = NULL;

  // TODO test
  /*char *dir1 = malloc(PATH_MAX * sizeof(char));
  char *dir2 = malloc(PATH_MAX * sizeof(char));
  char *fil1 = malloc(PATH_MAX * sizeof(char));
  char *fil2 = malloc(PATH_MAX * sizeof(char));
  char *fil3 = malloc(PATH_MAX * sizeof(char));
  char *fil4 = malloc(PATH_MAX * sizeof(char));
  *strcpy(dir1,
         "/tmp/progetto/bin/directory del mio paese1");
  strcpy(dir2,
         "/tmp/progetto/bin/directory 2");
  strcpy(fil1,
         "/tmp/progetto/bin/file del tuo paese 1");
  strcpy(fil2,
         "/tmp/progetto/bin/file 2");
  strcpy(fil2,
         "/tmp/progetto/bin/file 2");
  strcpy(fil3,
         "/tmp/progetto/bin/file 3");
  strcpy(fil4,
         "/tmp/progetto/bin/file 4");

  push(ui->directories, dir1);
  push(ui->directories, dir2);
  push(ui->files, fil1);
  push(ui->files, fil2);
  push(ui->files, fil3);
  push(ui->files, fil4); */
  // TODO test ended
  return ui;
}

void destroyUserInput(UserInput userInput) {
  destroyList(userInput->paths, free);
  destroyList(userInput->results, free);
  destroyList(userInput->directories, free);
  destroyList(userInput->files, free);
  // TODO double free nel caso ultima operazione sia una tree
  free(userInput->tree);
  free(userInput->table);
}

void *userInputLoop(void *ptr);
int readDirectives(List paths, int *numManager, int *numWorker, char *cwd);

void *writeFifoLoop(void *ptr);
void *readFifoLoop(void *ptr);
int sendDirectives(int fd, char *path, int *numManager, int *numWorker);
int readResult(List pathResults, char *cwd);
int sendResult(int fd, List pathResults);
int updateTree(char *path);
int sendTree(int fd, char *treePath);
int readTree(int readOperation, int fd, List directories, List files,
             char *cwd);
int readTable(int fd, unsigned long long *table);
void writeStats(unsigned long long *table);

int main(int argc, char **argv) {
  int managers;
  int workers;
  char *writeFifo = "/tmp/reporterToAnalyzer";
  char *readFifo = "/tmp/analyzerToReporter";
  char *cwd = malloc(PATH_MAX * sizeof(char));
  // TODO test only
  getcwd(cwd, PATH_MAX);
  /* remove(writeFifo); */
  int rc_fi = mkfifo(writeFifo, 0666);
  /* int rc_fi2 = mkfifo(readFifo, 0666); */
  UserInput userInput;
  userInput = newUserInput();
  int iter1;
  int iter2;
  int iter3;

  pthread_t userInputThraed;
  pthread_t writeFifoThraed;
  pthread_t readFifoThread;

  userInput_t input;
  pthread_mutex_init(&input.mutex, NULL);
  input.userInput = userInput;
  input.writeFifo = writeFifo;
  input.readFifo = readFifo;
  input.cwd = cwd;

  int rc_t = SUCCESS;

  // TUI
  int n = 3;
  int m = 4;
  int mode = FANCY_MODE;
  List args = newList();
  if (args == NULL)
    rc_t = MALLOC_FAILURE;

  int rc_opt = optionsHandler(args, argc, argv, &n, &m, &mode);
  rc_t = rc_opt;

  if (rc_t == SUCCESS) {
    printf("the programm start with n=%d e m=%d\n", n, m);
    int *width = malloc(sizeof(int));
    int *heigth = malloc(sizeof(int));

    getHeigth(heigth);
    getWidth(width);
    if (*heigth < 28 || *width < 89) {
      printf("For a better user experience we suggest you to increase the size "
             "of "
             "the terminal window. We develped a fancy graphics with a better "
             "UI\n");
      mode = NORMAL_MODE;
      printf("we give to you the possibility to read this message for 5s\n");
      /* sleep(1); */
      /* printf("we give to you the possibility to read this message for
       * 4s\n");
       */
      /* sleep(1); */
      /* printf("we give to you the possibility to read this message for
       * 3s\n");
       */
      /* sleep(1); */
      /* printf("we give to you the possibility to read this message for
       * 2s\n");
       */
      /* sleep(1); */
      /* printf("we give to you the possibility to read this message for
       * 1s\n");
       */
      /* sleep(1); */
      clear();
      moveCursor(0, 0);
      fflush(stdout);
    }
    if (mode == FANCY_MODE) {
      pthread_t graphics, inputGraphics;
      int iret1, iret2, iter3, iter4;
      set_input_mode();
      Screen screen = newScreen(*width, *heigth);

      input.screen = screen;

      iret1 = pthread_create(&graphics, NULL, graphicsLoop, (void *)&input);
      iret2 = pthread_create(&inputGraphics, NULL, inputLoop, (void *)&input);
      iter3 =
          pthread_create(&writeFifoThraed, NULL, writeFifoLoop, (void *)&input);
      iter4 =
          pthread_create(&readFifoThread, NULL, readFifoLoop, (void *)&input);

      pthread_join(graphics, NULL);
      pthread_join(inputGraphics, NULL);
      pthread_join(writeFifoThraed, NULL);
      pthread_join(readFifoThread, NULL);
      clear();

      // CASI LIMITE
      // 87 28
      fflush(stdout);
      clear();

      free(width);
      free(heigth);
      destroyScreen(screen);
    } else {
      iter1 =
          pthread_create(&userInputThraed, NULL, userInputLoop, (void *)&input);
      iter2 =
          pthread_create(&writeFifoThraed, NULL, writeFifoLoop, (void *)&input);
      iter3 =
          pthread_create(&readFifoThread, NULL, readFifoLoop, (void *)&input);
      pthread_join(userInputThraed, NULL);
      pthread_join(writeFifoThraed, NULL);
      pthread_join(readFifoThread, NULL);
    }
  } else {
    printError("Bad usage or malformed option");
    printf("%s\n", helpMsgV2);
  }

  destroyUserInput(userInput);

  return rc_t;
}

void *userInputLoop(void *ptr) {
  userInput_t *input = (userInput_t *)ptr;

  int rc_t = SUCCESS;
  char *cmdMsg =
      "write:\n\t dire->to send directive to analyzer\n\t requ->to request "
      "the analisy on one or more files\n\t tree->i can't explain\n\t resu "
      "->display result of requested files";
  char *direMsg = "directive mode, write:\n\t filePath or folderPath then line "
                  "feed\n\t number of "
                  "manager then line feed\n\t nubmer fo worker then line feed";
  char *requMsg =
      "request mode, write:\n\t the file names of which you want the analisy "
      "the line feed\n\t requ to exit from requ mode then line feed";
  char *treeMsg =
      "tree mode, write:\n\t the file names of which you want the analisy";
  while (rc_t == SUCCESS) {
    printf("%s\n", cmdMsg);
    char *dst = malloc(PATH_MAX * sizeof(char));
    int bytesRead = read(0, dst, 5);
    if (bytesRead > 0) {
      if (strncmp(dst, "dire", 4) == 0) {
        printf("%s\n", direMsg);
        pthread_mutex_lock(&(input->mutex));
        rc_t = readDirectives(input->userInput->paths,
                              &(input->userInput->managers),
                              &(input->userInput->workers), input->cwd);
        pthread_mutex_unlock(&(input->mutex));
        clear();
        moveCursor(0, 0);
        fflush(stdout);
        if (rc_t == CAST_FAILURE) {
          printf("please insert a valid format\n\t -path\t -number of manager "
                 "(int)\n\t -number of worker (int)\n");
          rc_t = SUCCESS;
        }
      } else if (strncmp(dst, "requ", 4) == 0) {
        printf("%s\n", requMsg);
        pthread_mutex_lock(&(input->mutex));
        readResult(input->userInput->results, input->cwd);
        pthread_mutex_unlock(&(input->mutex));
        clear();
        moveCursor(0, 0);
        fflush(stdout);
        printf("we are processing your request, please wait\n");
      } else if (strncmp(dst, "tree", 4) == 0) {
        pthread_mutex_lock(&(input->mutex));
        updateTree(input->userInput->tree);
        pthread_mutex_unlock(&(input->mutex));
      } else if (strncmp(dst, "resu", 4) == 0) {
        pthread_mutex_lock(&(input->mutex));
        writeStats(input->userInput->table);
        pthread_mutex_unlock(&(input->mutex));
      }
    }
    usleep(50000);
  }
  printf("sono morto\n");
}

void writeStats(unsigned long long *table) {
  char *outString = malloc(PATH_MAX * sizeof(char));
  char *tmpString = malloc(PATH_MAX * sizeof(char));
  unsigned long long maiuscole = 0;
  unsigned long long minuscole = 0;
  unsigned long long punteg = 0;
  unsigned long long cifre = 0;
  unsigned long long tutto = 0;
  unsigned long long other = 0;

  int i;
  for (i = 0; i < NCHAR_TABLE; i++) {
    if (i >= 97 && i <= 122)
      maiuscole += table[i];
    else if (((i >= 32 && i <= 47)) || ((i >= 58 && i <= 64)) ||
             ((i >= 91 && i <= 96)) || ((i >= 123 && i <= 126)))
      punteg += table[i];
    else if (i >= 48 && i <= 57)
      cifre += table[i];
    else if (i >= 65 && i <= 90)
      minuscole += table[i];
    else
      other += table[i];
    tutto += table[i];
  }

  int percentageMaiuscole = 0;
  int percentageMinuscole = 0;
  int percentagePunteg = 0;
  int percentageCifre = 0;
  int percentageOther = 0;
  strcat(outString, "Statistics:\n");
  sprintf(tmpString, "\tTutto: %llu\n", tutto);
  strcat(outString, tmpString);
  if (tutto != 0 && maiuscole != 0) {
    percentageMaiuscole =
        (int)((long double)maiuscole / (long double)tutto * 100);
  }
  sprintf(tmpString, "\tMaiuscole: %llu  -- percentage over total: %d%%\n",
          maiuscole, percentageMaiuscole);
  strcat(outString, tmpString);
  if (tutto != 0 && minuscole != 0) {
    percentageMinuscole =
        (int)((long double)minuscole / (long double)tutto * 100);
  }
  sprintf(tmpString, "\tMinuscole: %llu  -- percentage over total: %d%%\n",
          minuscole, percentageMinuscole);
  strcat(outString, tmpString);
  if (tutto != 0 && punteg != 0) {
    percentagePunteg = (int)((long double)punteg / (long double)tutto * 100);
  }
  sprintf(tmpString, "\tPunteg: %llu  -- percentage over total: %d%%\n", punteg,
          percentagePunteg);
  strcat(outString, tmpString);
  if (tutto != 0 && cifre != 0) {
    percentageCifre = (int)((long double)cifre / (long double)tutto * 100);
  }
  sprintf(tmpString, "\tCifre: %llu  -- percentage over total: %d%%\n", cifre,
          percentageCifre);
  strcat(outString, tmpString);
  if (tutto != 0 && other != 0) {
    percentageOther = (int)((long double)other / (long double)tutto * 100);
  }
  sprintf(tmpString, "\tOther: %llu  -- percentage over total: %d%%\n", other,
          percentageOther);
  strcat(outString, tmpString);

  printf("%s\n", outString);
  free(outString);
}

int isAnalyzerAlive() {
  int rc_t = -1;
  int pid = fork();

  if (pid == 0) {
    execlp("ps -a | grep 'ricevente'", "ps -a | grep 'ricevente'", NULL);
  } else if (pid > 0) {
    waitpid(pid, &rc_t, WNOHANG);

    WEXITSTATUS(rc_t);
  }

  return rc_t;
}

void *writeFifoLoop(void *ptr) {
  userInput_t *input = (userInput_t *)ptr;

  int rc_t = SUCCESS;
  int fd;
  char *fifoPath = malloc(PATH_MAX * sizeof(char));
  int lastManager = 3;
  int lastWorker = 4;
  pthread_mutex_lock(&(input->mutex));
  strcpy(fifoPath, input->writeFifo);
  pthread_mutex_unlock(&(input->mutex));
  // TODO shamefull fifo open and close
  int pathsSizePlaceholder = 0;
  int managerCountPlaceholder = 3;
  int workerCountPlaceholder = 4;
  int resultsSizePlaceholder = 0;
  int toggledFlagPlaceholder = 0;
  int lastResultCount = 0;
  char firstCharTree = '\0';

  fd = open(fifoPath, O_WRONLY);
  pthread_mutex_lock(&(input->mutex));
  rc_t = sendTree(fd, input->cwd);
  pthread_mutex_unlock(&(input->mutex));
  while (rc_t == SUCCESS) {
    pthread_mutex_lock(&(input->mutex));
    pathsSizePlaceholder = input->userInput->paths->size;
    pthread_mutex_unlock(&(input->mutex));
    if (pathsSizePlaceholder > 0) {
      /* printf("va che non e vuota come pensavi\n"); */
      /* printf("sto per aprire la fifo avendo letto %s\n", path); */
      fprintf(stderr, "open numero 1\n");
      /* fd = open(fifoPath, O_WRONLY); */
      fprintf(stderr, "open numero 1 aperta\n");
      pthread_mutex_lock(&(input->mutex));
      char *path = front(input->userInput->paths);
      if (path != NULL) {
        fprintf(stderr, "ciclo\n");
        if (fd > 0) {
          int rc_po = pop(input->userInput->paths);
          if (rc_po == -1) {
            /* printf("la pop e' andata una merda\n"); */
          } else {
            /* printf("la pop e' andata una in maniera magistrale\n"); */
          }
          rc_t = sendDirectives(fd, path, &(input->userInput->managers),
                                &(input->userInput->workers));
          rc_t = sendTree(fd, input->cwd);
          fprintf(stderr, "ho inviato la seconda volta\n");
          // TODO debug only
          /* if (strcmp(path, input->cwd) == 0) { */
          /*   sendTree(fd, input->cwd); */
          /* } */
        }
        free(path);
      }
      pthread_mutex_unlock(&(input->mutex));
      fprintf(stderr, "close numero 1\n");
      /* close(fd); */
      fprintf(stderr, "close numero 1 chiusa\n");
    }

    pthread_mutex_lock(&(input->mutex));
    managerCountPlaceholder = input->userInput->managers;
    pthread_mutex_unlock(&(input->mutex));
    if (managerCountPlaceholder != lastManager) {
      lastManager = managerCountPlaceholder;
      fprintf(stderr, "open numero 2\n");
      /* fd = open(fifoPath, O_WRONLY); */
      fprintf(stderr, "open numero 2 aperta\n");
      pthread_mutex_lock(&(input->mutex));
      if (fd > 0) {
        rc_t = sendDirectives(fd, "///", &(input->userInput->managers),
                              &(input->userInput->workers));
      }
      pthread_mutex_unlock(&(input->mutex));
      fprintf(stderr, "close numero 2\n");
      /* close(fd); */
      fprintf(stderr, "close numero 2 chiusa\n");
    }

    pthread_mutex_lock(&(input->mutex));
    workerCountPlaceholder = input->userInput->workers;
    pthread_mutex_unlock(&(input->mutex));
    if (workerCountPlaceholder != lastWorker) {
      /* printf("last: %d, new: %d\n", lastWorker, input->userInput->workers);
       */
      lastWorker = workerCountPlaceholder;
      fprintf(stderr, "open numero 3\n");
      /* fd = open(fifoPath, O_WRONLY); */
      fprintf(stderr, "open numero 3 aperta\n");
      pthread_mutex_lock(&(input->mutex));
      if (fd > 0) {
        /* rc_t = sendDirectives(fd, "///", &(input->userInput->managers), */
        /*                       &(input->userInput->workers)); */
      }
      pthread_mutex_unlock(&(input->mutex));
      fprintf(stderr, "close numero 3\n");
      /* close(fd); */
      fprintf(stderr, "close numero 3 chiusa\n");
    }

    pthread_mutex_lock(&(input->mutex));
    resultsSizePlaceholder = input->userInput->results->size;
    toggledFlagPlaceholder = input->userInput->toggledChanged;
    pthread_mutex_unlock(&(input->mutex));
    if (resultsSizePlaceholder != lastResultCount ||
        toggledFlagPlaceholder == 1) {
      lastResultCount = resultsSizePlaceholder;
      fprintf(stderr, "open numero 4\n");
      /* fd = open(fifoPath, O_WRONLY); */
      fprintf(stderr, "open numero 4 aperta\n");
      pthread_mutex_lock(&(input->mutex));
      input->userInput->toggledChanged = 0;
      if (fd > 0) {
        // printf("sto per entrar nel invio classico\n");
        rc_t = sendResult(fd, input->userInput->results);
        // printf("entrato\n");
      }
      pthread_mutex_unlock(&(input->mutex));
      fprintf(stderr, "close numero 4\n");
      /* close(fd); */
      fprintf(stderr, "close numero 4 chiusa\n");
    }

    pthread_mutex_lock(&(input->mutex));
    firstCharTree = input->userInput->tree[0];
    pthread_mutex_unlock(&(input->mutex));
    if (firstCharTree != '\0') {
      fprintf(stderr, "open numero 5\n");
      /* fd = open(fifoPath, O_WRONLY); */
      fprintf(stderr, "open numero 5 aperta\n");
      pthread_mutex_lock(&(input->mutex));
      if (fd > 0) {
        fprintf(stderr, "Richiesta del tree %s\n", input->userInput->tree);
        rc_t = sendTree(fd, input->userInput->tree);
        fprintf(stderr, "Send tree dice %d\n", rc_t);
        if (rc_t == SUCCESS) {
          // strcpy(input->userInput->tree, input->cwd);
          // fprintf(stderr, "Cambio working directory\n");
          input->userInput->tree[0] = '\0';
          chdir(input->cwd);
          fprintf(stderr, "INPUT CWD: %s\n", input->cwd);
        }
      }
      pthread_mutex_unlock(&(input->mutex));
      fprintf(stderr, "close numero 5\n");
      /* close(fd); */
      fprintf(stderr, "close numero 5 chiusa\n");
    }
    usleep(500);
  }
  close(fd);
  free(fifoPath);
  fprintf(stderr, "MORTO READ LOOP\n");
}

void readString(int fd, char *dst) {
  int index = 0;
  int byteRead = -1;
  char charRead = 'a';

  while (byteRead != 0 && charRead != '\0') {
    byteRead = readChar(fd, &charRead);
    if (byteRead != 0) {
      dst[index++] = charRead;
    }
  }
}

void *readFifoLoop(void *ptr) {
  userInput_t *input = (userInput_t *)ptr;

  int rc_t = SUCCESS;
  int fd;
  char *fifoPath = malloc(PATH_MAX * sizeof(char));
  pthread_mutex_lock(&(input->mutex));
  strcpy(fifoPath, input->readFifo);
  pthread_mutex_unlock(&(input->mutex));

  while (1) {
    if (access(fifoPath, F_OK) == 0) {
      int fd = open(fifoPath, O_RDONLY);
      /* remove("/tmp/analyzerToReporter"); */
      rc_t = SUCCESS;
      while (rc_t == SUCCESS) {
        printf("Sono epico in read file loop\n");
        int readOperationFlag = 0;
        char *dst = malloc(PATH_MAX * sizeof(char));
        dst[0] = '\0';
        readString(fd, dst);
        fprintf(stderr, "Ho letto dalla fifo %s\n", dst);
        if (strcmp(dst, "tree") == 0) {
          fprintf(stderr, "E' TREE\n");
          dst[0] = '\0';
          readString(fd, dst);
          int castedNumber;
          int rc_cast = sscanf(dst, "%d", &castedNumber);
          fprintf(stderr, "Casted number = %d\n", castedNumber);
          if (rc_cast != EOF) {
            List tmpDirs = newList();
            List tmpFiles = newList();
            // TODO check allocation
            char *tmpCwd = malloc(PATH_MAX * sizeof(char));
            if (tmpFiles == NULL || tmpDirs == NULL)
              rc_t = MALLOC_FAILURE;
            else {
              pthread_mutex_lock(&(input->mutex));
              strcpy(tmpCwd, input->cwd);
              pthread_mutex_unlock(&(input->mutex));
              rc_t = readTree(castedNumber, fd, tmpDirs, tmpFiles, tmpCwd);
              fprintf(stderr, "Muoio dopo read tree\n");
              pthread_mutex_lock(&(input->mutex));
              destroyList(input->userInput->directories, free);
              destroyList(input->userInput->files, free);
              fprintf(
                  stderr,
                  "Muoio dopo la distruzione delle liste, epico finito male\n");
              input->userInput->directories = tmpDirs;
              input->userInput->files = tmpFiles;
              pthread_mutex_unlock(&(input->mutex));
            }
          }
        } else if (strcmp(dst, "tabl") == 0) {
          fprintf(stderr, "E' TABL");
          unsigned long long *tmpTable =
              calloc(NCHAR_TABLE, sizeof(unsigned long long));
          readTable(fd, tmpTable);
          fprintf(stderr, "SE NON ESCO SONO FOTTUTO!!!\n");
          pthread_mutex_lock(&(input->mutex));
          int i = 0;
          for (i = 0; i < NCHAR_TABLE; i++) {
            input->userInput->table[i] = tmpTable[i];
          }
          pthread_mutex_unlock(&(input->mutex));
          fprintf(stderr, "DIOCANE SONO ANCORA QUA!!!\n");
          free(tmpTable);
        } else {
          rc_t = FAILURE;
        }

        usleep(500);
      }
      close(fd);
    }
    usleep(500);
  }
  free(fifoPath);
  fprintf(stderr, "MORTO READ LOOP\n");
}

int readDirectives(List paths, int *numManager, int *numWorker, char *cwd) {
  int rc_t = SUCCESS;
  char readBuffer[2] = "a";
  char *newPath = malloc(PATH_MAX * sizeof(char));
  int rc_al = checkAllocationError(newPath);
  if (rc_al < SUCCESS) {
    rc_t = MALLOC_FAILURE;
  }
  char nWorker[PATH_MAX];
  char nManager[PATH_MAX];
  int counter = 0;
  int numberManager = 0;
  int numberWorker = 0;

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
    nManager[counter++] = readBuffer[0];
  } while (readBuffer[0] != '\0' && readBuffer[0] != '\n');
  nManager[counter] = '\0';

  int rc_sc = sscanf(nManager, "%d", &numberManager);
  if (rc_sc == 0 || (numberManager == 9 && strcmp(nManager, "9") == 0)) {
    rc_t = CAST_FAILURE;
  }

  counter = 0;
  do {
    int rc = readChar(READ_CHANNEL, readBuffer);
    nWorker[counter++] = readBuffer[0];
  } while (readBuffer[0] != '\0' && readBuffer[0] != '\n');
  nWorker[counter] = '\0';

  int rc_sc2 = sscanf(nWorker, "%d", &numberWorker);
  if (rc_sc2 == 0 || (numberWorker == 9 && strcmp(nWorker, "9") == 0)) {
    rc_t = CAST_FAILURE;
  }

  if (newPath[0] == '\0' || nWorker[0] == '\0') {
    char *msgErr = (char *)malloc(300);
    int rc_ca = checkAllocationError(msgErr);
    if (rc_ca < 0) {
      printError("I can't allocate memory");
    } else {
      sprintf(msgErr, "inisde reporter with pid: %d", getpid());
      printError(msgErr);
      free(msgErr);
    }
  } else if (rc_t == SUCCESS) {
    char *newAbsolutePath = malloc(PATH_MAX * sizeof(char));
    int rc_al = checkAllocationError(newAbsolutePath);
    if (rc_al == SUCCESS) {
      if (newPath[0] != '/') {
        strcat(newAbsolutePath, cwd);
        strcat(newAbsolutePath, "/");
        strcat(newAbsolutePath, newPath);
        // printf("Path tot: %s\n", newAbsolutePath);
      } else {
        strcpy(newAbsolutePath, newPath);
      }
      free(newPath);
      enqueue(paths, newAbsolutePath);
    } else {
      rc_t = MALLOC_FAILURE;
    }
    *numManager = numberManager;
    *numWorker = numberWorker;
  }

  return rc_t;
}

int sendDirectives(int fd, char *path, int *numManager, int *numWorker) {
  int rc_t = SUCCESS;

  char *nManager = malloc(PATH_MAX * sizeof(char));
  int rc_al = checkAllocationError(nManager);
  char *nWorker = malloc(PATH_MAX * sizeof(char));
  int rc_al2 = checkAllocationError(nWorker);

  int rc_sp = sprintf(nManager, "%d", *numManager);
  int rc_sp2 = sprintf(nWorker, "%d", *numWorker);

  if (rc_sp >= SUCCESS && rc_al2 >= SUCCESS && fd > SUCCESS) {
    int rc_wr = SUCCESS;
    int rc_wr1 = writeDescriptor(fd, path);
    int rc_wr2 = writeDescriptor(fd, nManager);
    int rc_wr3 = writeDescriptor(fd, nWorker);
    int rc_wr4 = writeDescriptor(fd, "dire");

    if (rc_wr < SUCCESS || rc_wr2 < SUCCESS || rc_wr3 < SUCCESS ||
        rc_wr4 < SUCCESS)
      rc_t = SEND_FAILURE;
  } else
    rc_t = SEND_FAILURE;

  if (rc_al == SUCCESS)
    free(nManager);
  else
    rc_t = MALLOC_FAILURE;

  if (rc_al2 == SUCCESS)
    free(nWorker);
  else
    rc_t = MALLOC_FAILURE;

  if (fd < SUCCESS)
    rc_t = SEND_FAILURE;
  else {
    /* closeDescriptor(fd); */
  }

  return rc_t;
}

int readResult(List pathResults, char *cwd) {
  int rc_t = SUCCESS;
  int endFlag = 1;

  while (endFlag && rc_t == SUCCESS) {
    char *path = malloc(PATH_MAX * sizeof(char));
    int index = 0;
    char charRead = 'a';
    int bytesRead = -1;
    rc_t = checkAllocationError(path);
    while (bytesRead != 0 && charRead != '\n') {
      bytesRead = readChar(0, &charRead);
      if (bytesRead > 0)
        path[index++] = charRead;
    }
    path[--index] = '\0';

    if (strncmp(path, "requ", 4) == 0) {
      endFlag = 0;
    } else if (path[0] != '\0' && rc_t == SUCCESS) {
      char *newResult = malloc(PATH_MAX * sizeof(char));
      int rc_al = checkAllocationError(newResult);
      if (rc_al == SUCCESS) {
        if (path[0] != '/') {
          strcat(newResult, cwd);
          strcat(newResult, "/");
          strcat(newResult, path);
        } else {
          strcpy(newResult, path);
        }
        free(path);
        rc_t = enqueue(pathResults, newResult);
      } else {
        rc_t = MALLOC_FAILURE;
      }
    }
  }

  return rc_t;
}

int sendResult(int fd, List pathResults) {
  fprintf(stderr, "Sono entrato nella malfamatissima SEND RESULT\n");
  int rc_t = SUCCESS;
  int old;
  int index = 0;
  int rc_al = SUCCESS;
  char *tmpPath = (char *)malloc(PATH_MAX * sizeof(char));
  rc_al = checkAllocationError(tmpPath);
  if (rc_al != SUCCESS) {
    rc_t = MALLOC_FAILURE;
  }
  Node element = pathResults->head;
  while (element != NULL && rc_t == SUCCESS) {
    index = 0;
    char *path = element->data;
    if (path != NULL) {
      // TODO OVERFLOW da gestire
      while (path[index] != '\0') {
        old = index;
        index++;
        tmpPath[old] = path[index];
      }
      fprintf(stderr, "Mando un path %s\n", path);
      fprintf(stderr, "senza slash %s\n", tmpPath);
      int rc_wr = writeDescriptor(fd, tmpPath);
      if (rc_wr < SUCCESS)
        rc_t = -1;
    }
    element = element->next;
  }

  free(tmpPath);

  writeDescriptor(fd, "//");

  return rc_t;
}

int updateTree(char *path) {
  int rc_t = SUCCESS;

  int index = 0;
  char charRead = 'a';
  int bytesRead = -1;
  while (bytesRead != 0 && charRead != '\n') {
    bytesRead = readChar(0, &charRead);
    if (bytesRead > 0)
      path[index++] = charRead;
  }
  path[--index] = '\0';

  return rc_t;
}

int sendTree(int fd, char *treePath) {
  fprintf(stderr, "Devo mandare lo stramaledetto TREEEEEE\n");
  char *tmpPath = malloc(sizeof(char) * PATH_MAX);
  int rc_t = SUCCESS;
  int old;
  int index = 0;
  int rc_al = checkAllocationError(tmpPath);
  if (rc_al != SUCCESS) {
    rc_t = MALLOC_FAILURE;
  }

  if(strcmp(treePath, "/") == 0){
    strcpy(tmpPath, treePath);
  } else {
    if (rc_t == SUCCESS && treePath[0] == '/') {
      // TODO overflow da gestire
      while (treePath[index] != '\0') {
        old = index;
        index++;
        tmpPath[old] = treePath[index];
      }
    }
  }

  fprintf(stderr, "Tree path vale: %s\n", tmpPath);
  int rc_wr = writeDescriptor(fd, tmpPath);
  if (rc_t == SUCCESS && rc_wr < SUCCESS)
    rc_t = -1;

  writeDescriptor(fd, "tree");

  // treePath[0] = '\0';

  free(tmpPath);
  return rc_t;
}

int readTree(int readOpeartion, int fd, List directories, List files,
             char *cwd) {
  int rc_t = SUCCESS;

  int bytesRead = -1;
  int charRead = 'a';

  while (readOpeartion != 0 && rc_t == SUCCESS) {
    char *dst = malloc(PATH_MAX * sizeof(char));
    dst[0] = '\0';
    readString(fd, dst);
    char *dst2 = malloc(PATH_MAX * sizeof(char));
    dst2[0] = '\0';
    readString(fd, dst2);
    if (strcmp(dst, "") != 0 && strcmp(dst2, "") != 0) {
      fprintf(stderr, "Ho letto come figlio il sig: %s di tipo %s\n", dst,
              dst2);
      char *elem = malloc(PATH_MAX * sizeof(char));
      strcpy(elem, cwd);
      strcat(elem, "/");
      strcat(elem, dst);
      if (strcmp(dst2, "d") == 0) {
        rc_t = push(directories, elem);
        fprintf(stderr, "Ho pushato una cartella %s \n", elem);
      } else {
        rc_t = push(files, elem);
        fprintf(stderr, "Ho pushato un file %s\n", elem);
      }
      readOpeartion--;
    } else
      free(dst);
    free(dst2);
  }

  return rc_t;
}

int readTable(int fd, unsigned long long *table) {
  fprintf(stderr, "Entro in read table\n");
  int rc_t = SUCCESS;
  int numbersToRead = NCHAR_TABLE;
  int index = 0;
  while (numbersToRead > 0) {
    char *dst = malloc(PATH_MAX * sizeof(char));
    readString(fd, dst);
    // printf("STAMPO %s\n", dst);
    unsigned long long count = 0;
    int rc_cast = sscanf(dst, "%llu", &count);
    if (rc_cast != EOF)
      table[index++] = count;
    else
      rc_t = CAST_FAILURE;

    numbersToRead--;
    free(dst);
  }

  return rc_t;
}
