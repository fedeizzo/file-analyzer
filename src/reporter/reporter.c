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
    "\t-n, --number      : specify the managers amount\n"
    "\t-m, --mumber      : specify the workers amount\n"
    "\t-f,,--normal-mode : normal mode without fancy graphic\n";

UserInput newUserInput() {
  UserInput ui = malloc(sizeof(struct UserInputStr));
  int rc_al = checkAllocationError(ui);
  int rc_al2 = SUCCESS;
  int rc_al3 = SUCCESS;
  int rc_al4 = SUCCESS;
  int rc_al5 = SUCCESS;
  int rc_al6 = SUCCESS;
  int rc_al7 = SUCCESS;

  if (rc_al == SUCCESS) {
    ui->paths = newList();
    ui->results = newList();
    ui->directories = newList();
    ui->files = newList();
    ui->tree = malloc(PATH_MAX * sizeof(char));
    ui->table = calloc(NCHAR_TABLE, sizeof(unsigned long long));

    ui->tree[0] = '\0';
    if (ui->paths == NULL)
      rc_al2 = -1;
    if (ui->results == NULL)
      rc_al3 = -1;
    if (ui->directories == NULL)
      rc_al4 = -1;
    if (ui->files == NULL)
      rc_al5 = -1;
    rc_al6 = checkAllocationError(ui->tree);
    rc_al7 = checkAllocationError(ui->table);
    ui->managers = 3;
    ui->workers = 4;
    ui->toggledChanged = 0;
  }

  if (rc_al < SUCCESS || rc_al2 < SUCCESS || rc_al3 < SUCCESS ||
      rc_al4 < SUCCESS || rc_al5 < SUCCESS || rc_al6 < SUCCESS ||
      rc_al7 < SUCCESS)
    ui = NULL;
  return ui;
}

void destroyUserInput(UserInput userInput) {
  destroyList(userInput->paths, free);
  destroyList(userInput->results, free);
  destroyList(userInput->directories, free);
  destroyList(userInput->files, free);
  if (userInput->tree != NULL)
    free(userInput->tree);
  free(userInput->table);
}

/**
 * Thread for user input in normal mode
 *
 * args:
 *    void *ptr: pointer to shared resources
 */
void *userInputLoop(void *ptr);

/**
 * Thread that writes in fifo directed to analyzer
 *
 * args:
 *    void *ptr: pointer to shared resources
 */
void *writeFifoLoop(void *ptr);

/**
 * Thread that reads in fifo directed to analyzer
 *
 * args:
 *    void *ptr: pointer to shared resources
 */
void *readFifoLoop(void *ptr);

/**
 * Reads directives from standard input
 *
 * args:
 *    List paths     : a list where new paths are saved
 *    int *numManager: a pointer where new manager number are saved
 *    int *numWorker : a pointer where new worker number are saved
 *    char *cwd      : a pointer where new cwd are saved
 *
 * returns:
 *    0 in case of success, otherwise negative
 */
int readDirectives(List paths, int *numManager, int *numWorker, char *cwd);

/**
 * Writes directives into the fifo
 *
 * args:
 *    int fd         : file descriptor
 *    List paths     : a list where new paths are saved
 *    int *numManager: a pointer where new manager number are saved
 *    int *numWorker : a pointer where new worker number are saved
 *
 * returns:
 *    0 in case of success, otherwise negative
 */
int sendDirectives(int fd, char *path, int *numManager, int *numWorker);

/**
 * Reads a result from fifo
 *
 * args:
 *    List pathResults: list where path results are saved
 *    char *cwd       : pointer where new cwd are saved
 *
 * returns:
 *    0 in case of success, otherwise negative
 */
int readResult(List pathResults, char *cwd);

/**
 * Writes results into the pipe
 *
 * args:
 *    int fd          : file descriptor
 *    List pathResults: list where path results are saved
 *
 * returns:
 *    0 in case of success, otherwise negative
 */
int sendResult(int fd, List pathResults);

/**
 * Writes the request into the fifo
 *
 * args:
 *    int fd       : file descriptor
 *    List treePath: the path of the node for which children are requested
 *
 * returns:
 *    0 in case of success, otherwise negative
 */
int sendTree(int fd, char *treePath);

/**
 * Reads result sent from analyzer through the fifo
 *
 * args:
 *    int readOpearation: the number of read operation
 *    int fd          : file descriptor
 *    List directories: list where directories are saved
 *    List files      : list where files results are saved
 *    char *cwd       : pointer where new cwd are saved
 *
 * returns:
 *    0 in case of success, otherwise negative
 */
int readTree(int readOperation, int fd, List directories, List files,
             char *cwd);

/**
 * Reads a table of toggled file from fifo
 *
 * args:
 *    int fd                   : file descriptor
 *    unsigned long long *table: array where table is saved
 *
 * returns:
 *    0 in case of success, otherwise negative
 */
int readTable(int fd, unsigned long long *table);

/**
 * Computes statistics and shows to user for normal mode
 *
 * args:
 *    unsigned long long *table: array where table is saved
 */
void writeStats(unsigned long long *table);

/**
 * Reads from a file descriptor until \0 char
 *
 * args:
 *    int fd   : file descriptor
 *    char *dst: destination of the read operation
 */
void readString(int fd, char *dst);

int main(int argc, char **argv) {
  int rc_t = SUCCESS;
  char *writeFifo = "/tmp/reporterToAnalyzer";
  char *readFifo = "/tmp/analyzerToReporter";
  char *cwd = malloc(PATH_MAX * sizeof(char));
  getcwd(cwd, PATH_MAX);
  int rc_fi = mkfifo(writeFifo, 0666);
  UserInput userInput;
  userInput = newUserInput();
  int iter1;
  int iter2;
  int iter3;

  int mode = FANCY_MODE;

  int rc_opt = optionsHandler(userInput->paths, cwd, argc, argv,
                              &userInput->workers, &userInput->managers, &mode);
  rc_t = rc_opt;

  pthread_t userInputThraed;
  pthread_t writeFifoThraed;
  pthread_t readFifoThread;

  userInput_t input;
  pthread_mutex_init(&input.mutex, NULL);
  input.userInput = userInput;
  input.writeFifo = writeFifo;
  input.readFifo = readFifo;
  input.cwd = cwd;

  // TUI

  if (rc_t == SUCCESS) {
    int *width = malloc(sizeof(int));
    int *heigth = malloc(sizeof(int));

    getHeigth(heigth);
    getWidth(width);
    if (*heigth < 28 || *width < 89 || mode == NORMAL_MODE) {
      printf("For a better user experience we suggest you to increase the size "
             "of "
             "the terminal window. We develped a fancy graphics with a better "
             "UI\n");
      mode = NORMAL_MODE;
      printf("we give to you the possibility to read this message for 5s\n");
      sleep(1);
      printf("we give to you the possibility to read this message for 4s\n");
      sleep(1);
      printf("we give to you the possibility to read this message for 3s\n");
      sleep(1);
      printf("we give to you the possibility to read this message for 2s\n");
      sleep(1);
      printf("we give to you the possibility to read this message for 1s\n");
      sleep(1);
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
  } else if (rc_t != HELP_PRINT) {
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
      "write:\n   dire -> to send directive to analyzer\n   requ -> to request "
      "the analisy on one or more files\n   resu "
      "-> display result of requested files\n   quit -> quit the programm";
  char *direMsg = "directive mode, write:\n   filePath or folderPath then line "
                  "feed\n   number of "
                  "manager then line feed\n   nubmer fo worker then line feed";
  char *requMsg =
      "request mode, write:\n   the files names of which you want the analisy "
      "the line feed\n   requ to exit from requ mode then line feed";
  while (rc_t == SUCCESS) {
    printf("%s\n", cmdMsg);
    char *dst = malloc(PATH_MAX * sizeof(char));
    rc_t = checkAllocationError(dst);
    int bytesRead = read(0, dst, 5);
    dst[bytesRead - 1] = '\0';
    if (bytesRead > 0 && rc_t == SUCCESS) {
      if (strcmp(dst, "dire") == 0) {
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
          printf("please insert a valid format\n   -path   -number of manager "
                 "(int)\n   -number of worker (int)\n");
          rc_t = SUCCESS;
        }
      } else if (strcmp(dst, "requ") == 0) {
        printf("%s\n", requMsg);
        pthread_mutex_lock(&(input->mutex));
        destroyList(input->userInput->results, free);
        input->userInput->results = newList();
        int i = 0;
        for (i = 0; i < NCHAR_TABLE; i++)
          input->userInput->table[i] = 0;
        if (input->userInput->results != NULL)
          readResult(input->userInput->results, input->cwd);
        pthread_mutex_unlock(&(input->mutex));
        clear();
        moveCursor(0, 0);
        fflush(stdout);
        printf("we are processing your request, please wait\n");
      } else if (strcmp(dst, "resu") == 0) {
        pthread_mutex_lock(&(input->mutex));
        writeStats(input->userInput->table);
        pthread_mutex_unlock(&(input->mutex));
      } else if (strcmp(dst, "quit") == 0) {
        clear();
        moveCursor(0, 0);
        exit(0);
      }
    }
    usleep(50000);
  }
  kill(getpid(), SIGKILL);
}

void writeStats(unsigned long long *table) {
  char *outString = malloc(PATH_MAX * sizeof(char));
  char *tmpString = malloc(PATH_MAX * sizeof(char));
  int rc_al = checkAllocationError(outString);
  int rc_al2 = checkAllocationError(tmpString);
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

  if (rc_al == SUCCESS && rc_al2 == SUCCESS) {
    double percentageMaiuscole = 0;
    double percentageMinuscole = 0;
    double percentagePunteg = 0;
    double percentageCifre = 0;
    double percentageOther = 0;
    strcat(outString, "Statistics:\n");
    sprintf(tmpString, "  Tutto: %llu\n", tutto);
    strcat(outString, tmpString);
    if (tutto != 0 && maiuscole != 0) {
      percentageMaiuscole =
          (double)((long double)maiuscole / (long double)tutto * 100);
    }
    sprintf(tmpString, "  Maiuscole: %llu  -- percentage over total: %.2f%%\n",
            maiuscole, percentageMaiuscole);
    strcat(outString, tmpString);
    if (tutto != 0 && minuscole != 0) {
      percentageMinuscole =
          (double)((long double)minuscole / (long double)tutto * 100);
    }
    sprintf(tmpString, "  Minuscole: %llu  -- percentage over total: %.2f%%\n",
            minuscole, percentageMinuscole);
    strcat(outString, tmpString);
    if (tutto != 0 && punteg != 0) {
      percentagePunteg =
          (double)((long double)punteg / (long double)tutto * 100);
    }
    sprintf(tmpString, "  Punteg: %llu  -- percentage over total: %.2f%%\n",
            punteg, percentagePunteg);
    strcat(outString, tmpString);
    if (tutto != 0 && cifre != 0) {
      percentageCifre = (double)((long double)cifre / (long double)tutto * 100);
    }
    sprintf(tmpString, "  Cifre: %llu  -- percentage over total: %.2f%%\n",
            cifre, percentageCifre);
    strcat(outString, tmpString);
    if (tutto != 0 && other != 0) {
      percentageOther = (double)((long double)other / (long double)tutto * 100);
    }
    sprintf(tmpString, "  Other: %llu  -- percentage over total: %.2f%%\n",
            other, percentageOther);
    strcat(outString, tmpString);

    printf("%s\n", outString);
    free(outString);
    free(tmpString);
  }
}

void *writeFifoLoop(void *ptr) {
  userInput_t *input = (userInput_t *)ptr;

  int rc_t = SUCCESS;
  int fd;
  char *fifoPath = malloc(PATH_MAX * sizeof(char));
  rc_t = checkAllocationError(fifoPath);
  int lastManager = 3;
  int lastWorker = 4;
  int pathsSizePlaceholder = 0;
  int managerCountPlaceholder = 3;
  int workerCountPlaceholder = 4;
  int resultsSizePlaceholder = 0;
  int toggledFlagPlaceholder = 0;
  int lastResultCount = 0;
  char firstCharTree = '\0';
  if (rc_t == SUCCESS) {
    pthread_mutex_lock(&(input->mutex));
    strcpy(fifoPath, input->writeFifo);
    pthread_mutex_unlock(&(input->mutex));
    if (access(fifoPath, F_OK) == 0) {
      fd = open(fifoPath, O_WRONLY);
      pthread_mutex_lock(&(input->mutex));
      rc_t = sendTree(fd, input->cwd);
      pthread_mutex_unlock(&(input->mutex));
    } else {
      rc_t = FIFO_FAILURE;
    }
  }

  while (rc_t == SUCCESS) {
    pthread_mutex_lock(&(input->mutex));
    pathsSizePlaceholder = input->userInput->paths->size;
    pthread_mutex_unlock(&(input->mutex));
    if (pathsSizePlaceholder > 0) {
      pthread_mutex_lock(&(input->mutex));
      char *path = front(input->userInput->paths);
      if (path != NULL) {
        if (fd > 0) {
          int rc_po = pop(input->userInput->paths);
          rc_t = sendDirectives(fd, path, &(input->userInput->managers),
                                &(input->userInput->workers));
          rc_t = sendTree(fd, input->cwd);
        }
        free(path);
      }
      pthread_mutex_unlock(&(input->mutex));
    }

    pthread_mutex_lock(&(input->mutex));
    managerCountPlaceholder = input->userInput->managers;
    pthread_mutex_unlock(&(input->mutex));
    if (managerCountPlaceholder != lastManager) {
      lastManager = managerCountPlaceholder;
      pthread_mutex_lock(&(input->mutex));
      if (fd > 0) {
        rc_t = sendDirectives(fd, "///", &(input->userInput->managers),
                              &(input->userInput->workers));
      }
      pthread_mutex_unlock(&(input->mutex));
    }

    pthread_mutex_lock(&(input->mutex));
    workerCountPlaceholder = input->userInput->workers;
    pthread_mutex_unlock(&(input->mutex));
    if (workerCountPlaceholder != lastWorker) {
      lastWorker = workerCountPlaceholder;
      pthread_mutex_lock(&(input->mutex));
      if (fd > 0) {
        rc_t = sendDirectives(fd, "///", &(input->userInput->managers),
                              &(input->userInput->workers));
      }
      pthread_mutex_unlock(&(input->mutex));
    }

    pthread_mutex_lock(&(input->mutex));
    resultsSizePlaceholder = input->userInput->results->size;
    toggledFlagPlaceholder = input->userInput->toggledChanged;
    pthread_mutex_unlock(&(input->mutex));
    if (resultsSizePlaceholder != lastResultCount ||
        toggledFlagPlaceholder == 1) {
      lastResultCount = resultsSizePlaceholder;
      pthread_mutex_lock(&(input->mutex));
      input->userInput->toggledChanged = 0;
      if (fd > 0) {
        rc_t = sendResult(fd, input->userInput->results);
      }
      pthread_mutex_unlock(&(input->mutex));
    }

    pthread_mutex_lock(&(input->mutex));
    firstCharTree = input->userInput->tree[0];
    pthread_mutex_unlock(&(input->mutex));
    if (firstCharTree != '\0') {
      pthread_mutex_lock(&(input->mutex));
      if (fd > 0) {
        rc_t = sendTree(fd, input->userInput->tree);
        if (rc_t == SUCCESS) {
          input->userInput->tree[0] = '\0';
          chdir(input->cwd);
        }
      }
      pthread_mutex_unlock(&(input->mutex));
    }
    usleep(500);
  }
  close(fd);
  free(fifoPath);
  kill(getpid(), SIGKILL);
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
  rc_t = checkAllocationError(fifoPath);

  if (rc_t == SUCCESS) {
    pthread_mutex_lock(&(input->mutex));
    strcpy(fifoPath, input->readFifo);
    pthread_mutex_unlock(&(input->mutex));
  }

  while (1) {
    if (access(fifoPath, F_OK) == 0) {
      int fd = open(fifoPath, O_RDONLY);
      rc_t = SUCCESS;
      while (rc_t == SUCCESS) {
        int readOperationFlag = 0;
        char *dst = malloc(PATH_MAX * sizeof(char));
        int rc_al = checkAllocationError(dst);
        if (rc_al == SUCCESS) {
          dst[0] = '\0';
          readString(fd, dst);
          if (strcmp(dst, "tree") == 0) {
            dst[0] = '\0';
            readString(fd, dst);
            int castedNumber;
            int rc_cast = sscanf(dst, "%d", &castedNumber);
            if (rc_cast != EOF) {
              List tmpDirs = newList();
              List tmpFiles = newList();
              char *tmpCwd = malloc(PATH_MAX * sizeof(char));
              int rc_al = checkAllocationError(tmpCwd);
              if (tmpFiles == NULL || tmpDirs == NULL || rc_al == FAILURE)
                rc_t = MALLOC_FAILURE;
              else {
                pthread_mutex_lock(&(input->mutex));
                strcpy(tmpCwd, input->cwd);
                pthread_mutex_unlock(&(input->mutex));
                rc_t = readTree(castedNumber, fd, tmpDirs, tmpFiles, tmpCwd);
                pthread_mutex_lock(&(input->mutex));
                destroyList(input->userInput->directories, free);
                destroyList(input->userInput->files, free);
                free(tmpCwd);
                input->userInput->directories = tmpDirs;
                input->userInput->files = tmpFiles;
                pthread_mutex_unlock(&(input->mutex));
              }
            }
          } else if (strcmp(dst, "tabl") == 0) {
            unsigned long long *tmpTable =
                calloc(NCHAR_TABLE, sizeof(unsigned long long));
            int rc_al = checkAllocationError(tmpTable);
            if (rc_al == SUCCESS) {
              readTable(fd, tmpTable);
              pthread_mutex_lock(&(input->mutex));
              int i = 0;
              for (i = 0; i < NCHAR_TABLE; i++) {
                input->userInput->table[i] = tmpTable[i];
              }
              pthread_mutex_unlock(&(input->mutex));
              free(tmpTable);
            } else {
              rc_t = MALLOC_FAILURE;
            }
          } else {
            rc_t = FAILURE;
          }
        }
        usleep(500);
      }
      close(fd);
    }
    usleep(500);
  }
  free(fifoPath);
}

int readDirectives(List paths, int *numManager, int *numWorker, char *cwd) {
  int rc_t = SUCCESS;
  char readBuffer[2] = "a";
  char *newPath = malloc(PATH_MAX * sizeof(char));
  int rc_al = checkAllocationError(newPath);
  if (rc_al < SUCCESS) {
    rc_t = MALLOC_FAILURE;
  } else {
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
      char *rlPath = malloc(PATH_MAX * sizeof(char));
      int rc_al = checkAllocationError(newResult);
      int rc_al2 = checkAllocationError(rlPath);
      if (rc_al == SUCCESS && rc_al2 == SUCCESS) {
        if (path[0] != '/') {
          strcat(newResult, cwd);
          strcat(newResult, "/");
          strcat(newResult, path);
          realpath(newResult, rlPath);
        } else {
          strcpy(rlPath, path);
        }
        free(path);
        rc_t = enqueue(pathResults, rlPath);
      } else {
        rc_t = MALLOC_FAILURE;
      }
    }
  }

  return rc_t;
}

int sendResult(int fd, List pathResults) {
  int rc_t = SUCCESS;
  int old;
  int index = 0;
  int rc_al = SUCCESS;
  char *tmpPath = (char *)malloc(PATH_MAX * sizeof(char));
  rc_al = checkAllocationError(tmpPath);
  if (rc_al != SUCCESS) {
    rc_t = MALLOC_FAILURE;
  } else {
    Node element = pathResults->head;
    while (element != NULL && rc_t == SUCCESS) {
      index = 0;
      char *path = element->data;
      if (path != NULL) {
        while (path[index] != '\0' && index < PATH_MAX) {
          old = index;
          index++;
          if (index < PATH_MAX)
            tmpPath[old] = path[index];
        }
        int rc_wr = writeDescriptor(fd, tmpPath);
        if (rc_wr < SUCCESS)
          rc_t = -1;
      }
      element = element->next;
    }

    free(tmpPath);

    writeDescriptor(fd, "//");
  }

  return rc_t;
}

int sendTree(int fd, char *treePath) {
  char *tmpPath = malloc(sizeof(char) * PATH_MAX);
  int rc_t = SUCCESS;
  int old;
  int index = 0;
  int rc_al = checkAllocationError(tmpPath);
  if (rc_al != SUCCESS) {
    rc_t = MALLOC_FAILURE;
  } else {
    if (strcmp(treePath, "/") == 0) {
      strcpy(tmpPath, treePath);
    } else {
      if (rc_t == SUCCESS && treePath[0] == '/') {
        while (treePath[index] != '\0' && index < PATH_MAX) {
          old = index;
          index++;
          if (index < PATH_MAX)
            tmpPath[old] = treePath[index];
        }
      }
    }

    int rc_wr = writeDescriptor(fd, tmpPath);
    if (rc_t == SUCCESS && rc_wr < SUCCESS)
      rc_t = -1;

    writeDescriptor(fd, "tree");

    free(tmpPath);
  }
  return rc_t;
}

int readTree(int readOpeartion, int fd, List directories, List files,
             char *cwd) {
  int rc_t = SUCCESS;

  int bytesRead = -1;
  int charRead = 'a';

  while (readOpeartion != 0 && rc_t == SUCCESS) {
    char *dst = malloc(PATH_MAX * sizeof(char));
    char *dst2 = malloc(PATH_MAX * sizeof(char));
    if (checkAllocationError(dst) == SUCCESS &&
        checkAllocationError(dst2) == SUCCESS) {
      dst[0] = '\0';
      readString(fd, dst);
      dst2[0] = '\0';
      readString(fd, dst2);
      if (strcmp(dst, "") != 0 && strcmp(dst2, "") != 0) {
        char *elem = malloc(PATH_MAX * sizeof(char));
        if (checkAllocationError(elem) == SUCCESS) {
          strcpy(elem, cwd);
          if (strcmp(cwd, "/") != 0) {
            strcat(elem, "/");
          }
          strcat(elem, dst);
          if (strcmp(dst2, "d") == 0) {
            rc_t = push(directories, elem);
          } else {
            rc_t = push(files, elem);
          }
          readOpeartion--;
        }
      }
      free(dst);
      free(dst2);
    }
  }

  return rc_t;
}

int readTable(int fd, unsigned long long *table) {
  int rc_t = SUCCESS;
  int numbersToRead = NCHAR_TABLE;
  int index = 0;
  while (numbersToRead > 0) {
    char *dst = malloc(PATH_MAX * sizeof(char));
    rc_t = checkAllocationError(dst);
    if (rc_t == SUCCESS) {
      readString(fd, dst);
      unsigned long long count = 0;
      int rc_cast = sscanf(dst, "%llu", &count);
      if (rc_cast != EOF)
        table[index++] = count;
      else
        rc_t = CAST_FAILURE;

      numbersToRead--;
      free(dst);
    }
  }

  return rc_t;
}
