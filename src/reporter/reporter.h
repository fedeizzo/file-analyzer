#ifndef __REPORTER_H__
#define __REPORTER_H__

#include "../list/list.h"
#include <pthread.h>

typedef struct UserInputStr {
  List paths;
  List results;
  int managers;
  int workers;
} *UserInput;

typedef struct {
  UserInput userInput;
  char *writeFifo;
  char *readFifo;
  pthread_mutex_t mutex;
} userInput_t;

UserInput newUserInput();

#endif
