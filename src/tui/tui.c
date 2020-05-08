#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "tui.h"

#define WRITE 1
#define READ 0

#define HOR_TABLE '-'
#define VER_TABLE '|'

char command[300] = "tutto";

/* Use this variable to remember original terminal attributes. */

struct termios saved_attributes;

void reset_input_mode(void) {
  tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes);
}

void set_input_mode(void) {
  struct termios tattr;
  char *name;

  /* Make sure stdin is a terminal. */
  if (!isatty(STDIN_FILENO)) {
    fprintf(stderr, "Not a terminal.\n");
    exit(EXIT_FAILURE);
  }

  /* Save the terminal attributes so we can restore them later. */
  tcgetattr(STDIN_FILENO, &saved_attributes);
  atexit(reset_input_mode);

  /* Set the funny terminal modes. */
  tcgetattr(STDIN_FILENO, &tattr);
  tattr.c_lflag &= ~(ICANON | ECHO); /* Clear ICANON and ECHO. */
  tattr.c_cc[VMIN] = 1;
  tattr.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);
}

Screen newScreen(int width, int heigth) {
  Screen screen = malloc(sizeof(struct ScreenStr));
  screen->rows = heigth;
  screen->cols = width;
  screen->grid = malloc(screen->cols * sizeof(char *));
  screen->updateGrid = malloc(screen->cols * sizeof(char *));
  screen->cmd = 5;
  int i = 0;
  for (i = 0; i < screen->cols; i++) {
    screen->grid[i] = malloc(heigth * sizeof(char));
    screen->updateGrid[i] = malloc(heigth * sizeof(char));
  }

  return screen;
}
void destroyScreen(Screen screen) {
  int i = 0;
  for (i = 0; i < screen->cols; i++) {
    free(screen->grid[i]);
    free(screen->updateGrid[i]);
  }

  free(screen->grid);
  free(screen);
}

int getWidth(int *width) {
  int fd[2];
  pipe(fd);
  int f = fork();
  if (f > 0) {
    close(fd[WRITE]);
    char *dst = malloc(300 * sizeof(char));
    read(fd[READ], dst, 300);
    sscanf(dst, "%d", width);
    wait(NULL);
    close(fd[READ]);
    free(dst);
  } else {
    close(fd[READ]);
    dup2(fd[WRITE], 1);
    close(fd[WRITE]);
    execlp("tput", "tput", "cols", NULL);
  }
  return 0;
}

int getHeigth(int *heigth) {
  int fd[2];
  pipe(fd);
  int f = fork();
  if (f > 0) {
    close(fd[WRITE]);
    char *dst = malloc(300 * sizeof(char));
    read(fd[READ], dst, 300);
    sscanf(dst, "%d", heigth);
    wait(NULL);
    close(fd[READ]);
    free(dst);
  } else {
    close(fd[READ]);
    dup2(fd[WRITE], 1);
    close(fd[WRITE]);
    execlp("tput", "tput", "lines", NULL);
  }
  return 0;
}

void clear() { printf("\033[2J"); }

void moveCursor(int x, int y) { printf("\033[%d;%dH", (y), (x)); }

void writeScreen(Screen screen, char *str, int x, int y) {
  int i = 0;
  while (str[i] != '\0') {
    screen->grid[x + i][y] = str[i];
    i++;
  }
}
void writeUpdateScreen(Screen screen, char *str, int x, int y) {
  int i = 0;
  while (str[i] != '\0') {
    screen->updateGrid[x + i][y] = str[i];
    i++;
  }
}
void insertBorder(Screen screen) {
  int i = 0;
  int c = 0;
  int r = 0;
  for (c = 0; c < screen->cols; c++) {
    for (r = 0; r < screen->rows; r++) {
      if ((r == 0 && c == 0) ||
          (c == screen->cols - 1 && r == screen->rows - 1) ||
          (c == screen->cols - 1 && r == 0) ||
          (c == 0 && r == screen->rows - 1) ||
          (c == screen->cols - 1 && r == 2) || (c == 0 && r == 2) ||
          (c == screen->cols - 1 && r == 4) || (c == 0 && r == 4))
        screen->grid[c][r] = '+';
      else if (r == 0 || r == 2 || r == 4 || (r == 6 && 0 < c && c < 19) ||
               (r == screen->rows - 5 && 19 < c && c < screen->cols - 1) ||
               r == screen->rows - 1)
        screen->grid[c][r] = HOR_TABLE;
      else if (c == 0 || (c == 19 && r != 1) || c == screen->cols - 1)
        screen->grid[c][r] = VER_TABLE;
      else
        screen->grid[c][r] = ' ';
    }
  }
  i = 0;
  int col = 21;
  int row = 7;
  char *msg = malloc(300 * sizeof(char));
  writeScreen(screen, "|char|cont|", 21, 6);
  for (i = 33; i < 127; i++) {
    sprintf(msg, "|    |    |");
    writeScreen(screen, msg, col, row++);
    if (row >= screen->rows - 5) {
      row = 7;
      col += 11;
      writeScreen(screen, "|char|cont|", col, 6);
    }
  }
  free(msg);
}

void writeScreenError(char *str) {
  moveCursor(21, 6);
  printf("\033[41m %s\033[m", str);
}

void writeScreenLog(int heigth, char *str) {
  moveCursor(21, heigth - 3);
  printf("\033[33m %s\033[m", str);
}

void printLog(Screen screen, int fd) {
  char dst[300];
  read(fd, dst, 300);
  writeScreen(screen, dst, 21, screen->rows - 3);
}
int commandFilter(const int cmd, const int counter) {
  int rc_t = -1;
  if (cmd == 1) {
    if (counter >= 65 && counter <= 90)
      rc_t = 0;
  } else if (cmd == 2) {
    if (counter >= 97 && counter <= 122)
      rc_t = 0;
  } else if (cmd == 3) {
    if (((counter >= 32 && counter <= 47)) ||
        ((counter >= 58 && counter <= 64)) ||
        ((counter >= 91 && counter <= 96)) ||
        ((counter >= 123 && counter <= 126)))
      rc_t = 0;
  } else if (cmd == 4) {
    if (counter >= 48 && counter <= 57)
      rc_t = 0;
  } else if (cmd == 5) {
    if (counter >= 33 && counter <= 126)
      rc_t = 0;
  }

  return rc_t;
}

Point draw(Screen screen) {
  int c = 0;
  int r = 0;
  Point lastCharWritten;
  lastCharWritten.y = 2;
  lastCharWritten.x = 3;
  clear();
  moveCursor(0, 0);
  for (r = 0; r < screen->rows; r++) {
    for (c = 0; c < screen->cols; c++) {
      /* if (screen->grid[c][r] - screen->updateGrid[c][r] != 0) { */
      /*   if (r == 1) */
      /*     lastCharWritten.x = lastCharWritten.x + 1; */
      /*   moveCursor(c + 1, r + 1); */
      /*   printf("%c", screen->grid[c][r]); */
      /*   screen->grid[c][r] = screen->updateGrid[c][r]; */
      /* } */
      if (r == screen->rows - 4 && c >= 21 && c <= 30) {
        printf("\033[33m%c\033[m", screen->grid[c][r]);
      } else if (r == 1 && c >= 2 && c <= 8) {
        printf("\033[32m%c\033[m", screen->grid[c][r]);
      } else {
        printf("%c", screen->grid[c][r]);
      }
      /* screen->grid[c][r] = screen->updateGrid[c][r]; */
    }
  }
  fflush(stdout);
  return lastCharWritten;
}

int getXPosAfterDraw(Screen screen) {
  int i = 2;
  /* while (screen->grid[i][3] != ' ') { */
  /*   i++; */
  /* } */
  return i;
}

void updateGrids(Screen screen) {
  int c = 0;
  int r = 0;
  for (r = 0; r < screen->rows; r++) {
    for (c = 0; c < screen->cols; c++) {
      screen->updateGrid[c][r] = screen->grid[c][r];
    }
  }
}

int checkCommand(char *cmd) {
  int rc_t;
  if (strcmp(cmd, "tree") == 0) {
    rc_t = 0;
  } else if (strcmp(cmd, "maiuscole") == 0) {
    rc_t = 0;
  } else if (strcmp(cmd, "minuscole") == 0) {
    rc_t = 0;
  } else if (strcmp(cmd, "punteg.") == 0) {
    rc_t = 0;
  } else if (strcmp(cmd, "cifre") == 0) {
    rc_t = 0;
  } else if (strcmp(cmd, "tutto") == 0) {
    rc_t = 0;
  } else {
    rc_t = -1;
  }

  return rc_t;
}

int checkFile(char *path) {
  int rc_t;
  int fd = open(path, O_RDONLY);
  if (fd != -1) {
    rc_t = 0;
    close(fd);
  } else
    rc_t = -1;

  return rc_t;
}

int getkey() {
  int character;
  struct termios orig_term_attr;
  struct termios new_term_attr;

  /* set the terminal to raw mode */
  tcgetattr(fileno(stdin), &orig_term_attr);
  memcpy(&new_term_attr, &orig_term_attr, sizeof(struct termios));
  new_term_attr.c_lflag &= ~(ECHO | ICANON);
  new_term_attr.c_cc[VTIME] = 0;
  new_term_attr.c_cc[VMIN] = 0;
  tcsetattr(fileno(stdin), TCSANOW, &new_term_attr);

  /* read a character from the stdin stream without blocking */
  /*   returns EOF (-1) if no character is available */
  character = fgetc(stdin);

  /* restore the original terminal attributes */
  tcsetattr(fileno(stdin), TCSANOW, &orig_term_attr);

  return character;
}
void *graphicsLoop(void *ptr) {
  screenMutex_t *p = (screenMutex_t *)(ptr);
  int rc_commandCheck = 0;
  int rc_fileCheck = 0;
  int fd[2];
  pipe(fd);
  dup2(fd[1], 2);

  clear();

  pthread_mutex_lock(&(p->mutex));
  insertBorder(p->screen);
  writeScreen(p->screen, "input: ", 2, 1);
  writeScreen(
      p->screen,
      " comandi: quit, n, m, tree, maiuscole, minuscole, punteg., cifre, tutto",
      1, 3);
  writeScreen(p->screen, " mode: ", 1, 7);
  writeScreen(p->screen, command, 7, 7);
  writeScreen(p->screen, " home ", 1, 8);
  writeScreen(p->screen, " > documents ", 1, 9);
  writeScreen(p->screen, "  > file.txt ", 1, 10);
  writeScreen(p->screen, "  > file2.txt ", 1, 11);
  draw(p->screen);
  writeScreen(p->screen, "LOG/ERROR", 21, p->screen->rows - 4);
  pthread_mutex_unlock(&(p->mutex));
  moveCursor(3, 2);
  Point lastWritten;

  int counter = 0;
  while (strcmp(command, "quit") != 0) {
    /* scanf("%s", command); */
    /* rc_commandCheck = checkCommand(command); */
    /* rc_fileCheck = checkFile(command); */
    /* if (rc_commandCheck == 0) { */
    /*   pthread_mutex_lock(&(p->mutex)); */
    /*   writeScreen(p->screen, command, 7, 7); */
    /*   pthread_mutex_unlock(&(p->mutex)); */
    /* } */

    pthread_mutex_lock(&(p->mutex));
    lastWritten = draw(p->screen);
    /* writeScreenLog(p->screen->rows, "LOG/ERROR"); */
    /* fprintf(stderr, "INFO: tuo figlio si e' suicidato %d", counter++); */
    /* printLog(p->screen, fd[0]); */
    /* if (rc_commandCheck != 0 && rc_fileCheck != 0) */
    /*   writeScreenError("Inserisci un comando o percorso valido"); */
    /* int xPos = getXPosAfterDraw(p->screen); */
    /* if (strcmp(command, "tree") == 0) */
    /*   moveCursor(3, 6); */
    /* else */
    /*   moveCursor(xPos, 2); */
    /* moveCursor(lastWritten.x, 2); */
    pthread_mutex_unlock(&(p->mutex));
    usleep(1000000);
  }
}

void drawInputLine(Screen screen) {
  int i = 0;
  moveCursor(10, 2);
  for (i = 9; i < screen->cols - 1; i++) {
    printf("%c", screen->grid[i][1]);
  }
  fflush(stdout);
}

void *inputLoop(void *ptr) {
  screenMutex_t *p = (screenMutex_t *)(ptr);
  int *oldWidth = malloc(sizeof(int));
  int *oldHeigth = malloc(sizeof(int));

  int *width = malloc(sizeof(int));
  int *heigth = malloc(sizeof(int));
  getHeigth(heigth);
  getWidth(width);
  *oldHeigth = *heigth;
  *oldWidth = *width;

  char *msg = malloc(300 * sizeof(char));
  pthread_mutex_lock(&(p->mutex));
  int counter = 9;
  p->screen->grid[counter][1] = '|';
  pthread_mutex_unlock(&(p->mutex));
  char key;
  char lastKey;
  while (strcmp(command, "quit") != 0) {
    key = EOF;
    getHeigth(heigth);
    getWidth(width);
    pthread_mutex_lock(&(p->mutex));
    updateGrids(p->screen);
    int i = 0;
    int col = 22;
    int row = 7;
    for (i = 33; i < 127; i++) {
      if (commandFilter(p->screen->cmd, i) != 0) {
        sprintf(msg, "    ");
        writeScreen(p->screen, msg, col, row);
        sprintf(msg, "    ");
        writeScreen(p->screen, msg, col + 5, row++);
      } else {
        sprintf(msg, "%c", i);
        writeScreen(p->screen, msg, col, row);
        sprintf(msg, "%d", rand() % 100);
        writeScreen(p->screen, msg, col + 5, row++);
      }
      if (row >= p->screen->rows - 5) {
        row = 7;
        col += 11;
      }
    }
    pthread_mutex_unlock(&(p->mutex));

    pthread_mutex_lock(&(p->mutex));
    key = getkey();
    if ((key > 31 && key <= 127) || key == 10) {
      if (key == 10) {
        int i;
        int cmdCounter = 0;
        char *cmd = malloc(10 * sizeof(char));
        for (i = 9; i < p->screen->cols - 1; i++) {
          if (cmdCounter < 10 && p->screen->grid[i][1] != '|') {
            cmd[cmdCounter++] = p->screen->grid[i][1];
          }
          p->screen->grid[i][1] = ' ';
        }
        cmd[cmdCounter] = '\0';
        /* p->screen->cmd = 4; */

        if (strncmp(cmd, "maiuscole ", 9) == 0) {
          writeScreen(p->screen, "input: ", 2, 1);
          p->screen->cmd = 1;
        } else if (strncmp(cmd, "minuscole ", 9) == 0) {
          writeScreen(p->screen, "input: ", 2, 1);
          p->screen->cmd = 2;
        } else if (strncmp(cmd, "punteg. ", 7) == 0) {
          writeScreen(p->screen, "input: ", 2, 1);
          p->screen->cmd = 3;
        } else if (strncmp(cmd, "cifre ", 5) == 0) {
          writeScreen(p->screen, "input: ", 2, 1);
          p->screen->cmd = 4;
        } else if (strncmp(cmd, "tutto ", 5) == 0) {
          writeScreen(p->screen, "input: ", 2, 1);
          p->screen->cmd = 5;
        } else if (strncmp(cmd, "quit ", 4) == 0) {
          clear();
          exit(0);
        } else if (strncmp(cmd, "n ", 1) == 0) {
          writeScreen(p->screen, "n    : ", 2, 1);
        } else if (strncmp(cmd, "m ", 1) == 0) {
          writeScreen(p->screen, "m    : ", 2, 1);
        }
        free(cmd);

        counter = 9;
        p->screen->grid[counter][1] = '|';
      } else if (key == 127) {
        if (counter > 9) {
          p->screen->grid[counter--][1] = ' ';
          p->screen->grid[counter][1] = '|';
        }
      } else if (key == '[' || key == ']') {
        lastKey = key;
      } else if (!((key == 65 || key == 66 || key == 67 || key == 68) &&
                   lastKey == '[')) {
        if (counter < p->screen->cols - 2) {
          p->screen->grid[counter++][1] = key;
          p->screen->grid[counter][1] = '|';
        }
      }
      drawInputLine(p->screen);
      lastKey = key;
    }
    pthread_mutex_unlock(&(p->mutex));
    /* if (*oldHeigth != *heigth || *oldWidth != *width) { */
    /*   pthread_mutex_lock(&(p->mutex)); */
    /*   destroyScreen(p->screen); */
    /*   p->screen = newScreen(*width, *heigth); */
    /*   clear(); */
    /*   pthread_mutex_unlock(&(p->mutex)); */
    /*   *oldHeigth = *heigth; */
    /*   *oldWidth = *width; */
    /* } */
    /* sleep(0); */
    usleep(50000);
  }
  free(msg);
}

int main(int argc, char **argv) {
  pthread_t graphics, input;
  int iret1, iret2;
  set_input_mode();
  screenMutex_t screen_mutex;
  int *width = malloc(sizeof(int));
  int *heigth = malloc(sizeof(int));

  getHeigth(heigth);
  getWidth(width);
  pthread_mutex_init(&screen_mutex.mutex, NULL);

  Screen screen = newScreen(*width, *heigth);

  screen_mutex.screen = screen;

  iret1 = pthread_create(&graphics, NULL, graphicsLoop, (void *)&screen_mutex);
  iret2 = pthread_create(&input, NULL, inputLoop, (void *)&screen_mutex);

  pthread_join(graphics, NULL);
  pthread_join(input, NULL);
  clear();

  // CASI LIMITE
  // 87 28
  fflush(stdout);
  clear();

  free(width);
  free(heigth);
  destroyScreen(screen);

  /* int key; */

  /* /1* initialize the random number generator *1/ */
  /* srand(time(NULL)); */

  /* for (;;) { */
  /*   key = getkey(); */
  /*   /1* terminate loop on ESC (0x1B) or Ctrl-D (0x04) on STDIN *1/ */
  /*   if (key == 0x61) { */
  /*     printf("%c", key); */
  /*     /1* break; *1/ */
  /*   } else { */
  /*     /1* print random ASCII character between 0x20 - 0x7F *1/ */
  /*     key = (rand() % 0x7F); */
  /*     /1* printf("%c", ((key < 0x20) ? (key + 0x20) : key)); *1/ */
  /*   } */
  /* } */

  return 0;
}
