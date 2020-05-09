#ifndef __TUI_H__
#define __TUI_H__

#include <pthread.h>

typedef struct ScreenStr {
  char **grid;
  int cols;
  int rows;
  int cmd;
} * Screen;

struct screenMutex {
  pthread_mutex_t mutex;
  Screen screen;
};

typedef struct screenMutex screenMutex_t;

Screen newScreen(int width, int heigth);
void destroyScreen(Screen screen);
int getWidth(int *width);
int getHeigth(int *heigth);
void clear();
void moveCursor(int x, int y);
void writeScreen(Screen screen, char *str, int x, int y);
void writeScreenError(char *str);
void writeScreenLog(int heigth, char *str);
int printLog(Screen screen, int fd);
int insertBorder(Screen screen);
void draw(Screen screen);
int checkCommand(char *cmd);
int checkFile(char *path);
void *graphicsLoop(void *ptr);
void *inputLoop(void *ptr);

#endif
