#ifndef __REPORTER_H__
#define __REPORTER_H__

#include "../list/list.h"
#include "../tui/tui.h"
#include <pthread.h>

typedef struct UserInputStr {
  List paths;
  List results;
  List directories;
  List files;
  int toggledChanged;
  unsigned long long *table;
  char *tree;
  int managers;
  int workers;
} *UserInput;

typedef struct {
  UserInput userInput;
  char *writeFifo;
  char *readFifo;
  char *cwd;
  pthread_mutex_t mutex;
  Screen screen;
} userInput_t;

UserInput newUserInput();

#endif
