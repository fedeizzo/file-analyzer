#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "../list/list.h"
#include "../wrapping/wrapping.h"
#include "tui.h"

#define FANCY_MODE 1
#define NORMAL_MODE 0

#define WRITE 1
#define READ 0

#define OK 0
#define CAST_FAILURE -3
#define HELP_PRINT -4
#define DOUBLE_OPTION_FAILURE -5
#define PIPE_FAILURE -6
#define GET_SIZE_FAILURE -7
#define PRINT_LOG_FAILURE -8
#define GRAPHIC_LOOP_FAILURE -9
#define INPUT_LOOP_FAILURE -10

#define HOR_TABLE '-'
#define VER_TABLE '|'

const char *helpMsg =
    "Usage: tui [OPTIONS] [FILES] [FOLDERS]\n\n"
    "OPTIONS:\n"
    "\t-h, --help        : print this message\n"
    "\t-n, --number     : specify the managers amount\n"
    "\t-m, --mumber      : specify the workers amount\n"
    "\t-f,,--normal-mode : normal mode without fancy graphic\n";

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
  // TODO make better control
  screen->rows = heigth;
  screen->cols = width;
  screen->grid = malloc(screen->cols * sizeof(char *));
  screen->cmd = 5;
  int i = 0;
  for (i = 0; i < screen->cols; i++) {
    screen->grid[i] = malloc(heigth * sizeof(char));
  }

  return screen;
}
void destroyScreen(Screen screen) {
  int i = 0;
  for (i = 0; i < screen->cols; i++) {
    free(screen->grid[i]);
  }

  free(screen->grid);
  free(screen);
}

int getWidth(int *width) {
  int rc_t = OK;
  int fd[2];
  int rc_pi = createUnidirPipe(fd);
  if (rc_pi == OK) {
    int f = fork();
    if (f > 0) {
      int rc_cl, rc_cl2, rc_al, rc_re, rc_ss;
      rc_cl = closeDescriptor(fd[WRITE]);
      char *dst = malloc(300 * sizeof(char));
      rc_al = checkAllocationError(dst);
      if (rc_al == OK) {
        rc_re = readDescriptor(fd[READ], dst, 300);
        rc_ss = sscanf(dst, "%d", width);
        wait(NULL);
        rc_cl2 = closeDescriptor(fd[READ]);
        free(dst);
        if (rc_cl < OK || rc_cl2 < OK || rc_re < OK || rc_ss == 0)
          rc_t = GET_SIZE_FAILURE;
      } else {
        rc_t = MALLOC_FAILURE;
      }
    } else {
      int rc_cl, rc_cl2, rc_du;
      rc_cl = closeDescriptor(fd[READ]);
      rc_du = createDup(fd[WRITE], 1);
      rc_cl2 = close(fd[WRITE]);
      if (rc_cl < OK || rc_cl2 < OK || rc_du < OK)
        rc_t = GET_SIZE_FAILURE;
      execlp("tput", "tput", "cols", NULL);
    }
  } else {
    rc_t = PIPE_FAILURE;
  }
  return rc_t;
}

int getHeigth(int *heigth) {
  int rc_t = OK;
  int fd[2];
  int rc_pi = createUnidirPipe(fd);
  if (rc_pi == OK) {
    int f = fork();
    if (f > 0) {
      int rc_cl, rc_cl2, rc_al, rc_re, rc_ss;
      rc_cl = closeDescriptor(fd[WRITE]);
      char *dst = malloc(300 * sizeof(char));
      rc_al = checkAllocationError(dst);
      if (rc_al == OK) {
        rc_re = readDescriptor(fd[READ], dst, 300);
        rc_ss = sscanf(dst, "%d", heigth);
        wait(NULL);
        rc_cl2 = closeDescriptor(fd[READ]);
        free(dst);
        if (rc_cl < OK || rc_cl2 < OK || rc_re < OK || rc_ss == 0)
          rc_t = GET_SIZE_FAILURE;
      } else {
        rc_t = MALLOC_FAILURE;
      }
    } else {
      int rc_cl, rc_cl2, rc_du;
      rc_cl = closeDescriptor(fd[READ]);
      rc_du = createDup(fd[WRITE], 1);
      rc_cl2 = close(fd[WRITE]);
      if (rc_cl < OK || rc_cl2 < OK || rc_du < OK)
        rc_t = GET_SIZE_FAILURE;
      execlp("tput", "tput", "lines", NULL);
    }
  } else {
    rc_t = PIPE_FAILURE;
  }
  return rc_t;
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

int insertBorder(Screen screen) {
  int rc_t = OK;

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
  int rc_al = checkAllocationError(msg);
  if (rc_al == OK) {
    writeScreen(screen, "  | cont   ", 21, 6);
    for (i = 33; i < 127; i++) {
      sprintf(msg, "  |        ");
      writeScreen(screen, msg, col, row++);
      if (row >= screen->rows - 5) {
        row = 7;
        col += 11;
        writeScreen(screen, "  | cont   ", col, 6);
      }
    }
    free(msg);
  } else {
    rc_t = MALLOC_FAILURE;
  }

  return rc_t;
}

void writeScreenError(char *str) {
  moveCursor(21, 6);
  printf("\033[41m %s\033[m", str);
}

void writeScreenLog(int heigth, char *str) {
  moveCursor(21, heigth - 3);
  printf("\033[33m %s\033[m", str);
}

int printLog(Screen screen, int fd) {
  char dst[300];
  int rc_rd = readDescriptor(fd, dst, 300);
  if (rc_rd == OK)
    writeScreen(screen, dst, 21, screen->rows - 3);
  else {
    rc_rd = PRINT_LOG_FAILURE;
  }
  return rc_rd;
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

void draw(Screen screen) {
  int c = 0;
  int r = 0;
  clear();
  moveCursor(0, 0);
  for (r = 0; r < screen->rows; r++) {
    for (c = 0; c < screen->cols; c++) {
      if (r == screen->rows - 4 && c >= 21 && c <= 30) {
        printf("\033[33m%c\033[m", screen->grid[c][r]);
      } else if (r == 1 && c >= 2 && c <= 8) {
        printf("\033[32m%c\033[m", screen->grid[c][r]);
      } else {
        printf("%c", screen->grid[c][r]);
      }
    }
  }
  fflush(stdout);
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
  // TODO choose if we use this
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

int initScreen(Screen screen) {
  int rc_t = insertBorder(screen);
  if (rc_t == OK) {
    writeScreen(screen, "input: ", 2, 1);
    writeScreen(screen,
                " comandi: quit, n, m, tree, maiuscole, minuscole, punteg., "
                "cifre, tutto",
                1, 3);
    writeScreen(screen, " mode: ", 1, 7);
    /* writeScreen(p->screen, command, 7, 7); */
    writeScreen(screen, " home ", 1, 8);
    writeScreen(screen, " > documents ", 1, 9);
    writeScreen(screen, "  > file.txt ", 1, 10);
    writeScreen(screen, "  > file2.txt ", 1, 11);
    draw(screen);
    writeScreen(screen, "LOG/ERROR", 21, screen->rows - 4);
  }

  return rc_t;
}

void *graphicsLoop(void *ptr) {
  screenMutex_t *p = (screenMutex_t *)(ptr);
  int rc_t = OK;
  int rc_pi, rc_du, rc_in;
  int rc_commandCheck = 0;
  int rc_fileCheck = 0;
  int fd[2];

  // TODO add support for this log
  rc_pi = createUnidirPipe(fd);
  if (rc_pi == OK)
    rc_du = dup2(fd[1], 2);

  clear();

  pthread_mutex_lock(&(p->mutex));
  rc_in = initScreen(p->screen);
  pthread_mutex_unlock(&(p->mutex));
  moveCursor(3, 2);

  int counter = 0;
  if (rc_pi < OK || rc_du < OK || rc_in < OK)
    rc_t = GRAPHIC_LOOP_FAILURE;
  while (rc_t == OK) {
    pthread_mutex_lock(&(p->mutex));
    draw(p->screen);
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

void updateTable(Screen screen) {
  int rc_t = OK;
  char *msg = malloc(300 * sizeof(char));

  int rc_al = checkAllocationError(msg);
  if (rc_al < OK)
    rc_t = MALLOC_FAILURE;

  int i = 0;
  int col = 22;
  int row = 7;
  for (i = 33; i < 127; i++) {
    if (commandFilter(screen->cmd, i) != 0) {
      sprintf(msg, " ");
      writeScreen(screen, msg, col, row);
      sprintf(msg, "         ");
      writeScreen(screen, msg, col + 2, row++);
    } else {
      int rndChoose = rand() % 2;
      int rnd;
      if (rndChoose == 0)
        rnd = rand() % 1000;
      else
        rnd = rand() % 100;
      sprintf(msg, "%c", i);
      writeScreen(screen, msg, col, row);
      sprintf(msg, "%d", rnd);
      if (rnd >= 1000000)
        writeScreen(screen, msg, col + 2, row++);
      else
        writeScreen(screen, msg, col + 3, row++);
    }
    if (row >= screen->rows - 5) {
      row = 7;
      col += 11;
    }
  }
  free(msg);
}

void *inputLoop(void *ptr) {
  screenMutex_t *p = (screenMutex_t *)(ptr);
  int rc_t = OK;
  int rc_al, rc_al2, rc_al3, rc_al4, rc_si, rc_si2, rc_in = OK;
  int *oldWidth = malloc(sizeof(int));
  rc_al = checkAllocationError(oldWidth);
  int *oldHeigth = malloc(sizeof(int));
  rc_al2 = checkAllocationError(oldHeigth);

  int *width = malloc(sizeof(int));
  rc_al3 = checkAllocationError(width);
  int *heigth = malloc(sizeof(int));
  rc_al4 = checkAllocationError(heigth);
  if (rc_al == OK && rc_al2 == OK && rc_al3 == OK && rc_al4 == OK) {
    rc_si = getHeigth(heigth);
    rc_si2 = getWidth(width);
    *oldHeigth = *heigth;
    *oldWidth = *width;
    if (rc_si < OK || rc_si2 < OK)
      rc_t = GET_SIZE_FAILURE;
  } else
    rc_t = MALLOC_FAILURE;

  pthread_mutex_lock(&(p->mutex));
  int counter = 9;
  p->screen->grid[counter][1] = '|';
  pthread_mutex_unlock(&(p->mutex));

  char key;
  char lastKey;
  while (rc_t == OK) {
    key = EOF;
    pthread_mutex_lock(&(p->mutex));
    updateTable(p->screen);
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

    rc_si = getHeigth(heigth);
    rc_si2 = getWidth(width);
    if (rc_si < OK || rc_si2 < OK)
      rc_t = GET_SIZE_FAILURE;
    if ((*oldHeigth != *heigth || *oldWidth != *width) && rc_t == OK) {
      pthread_mutex_lock(&(p->mutex));
      while ((*heigth < 28 || *width < 87) && rc_t == OK) {
        clear();
        printf(
            "window dimension is to small, please increase window size or "
            "luanch programma with -f option to use without fancy graphic at "
            "any size\n");
        rc_si = getHeigth(heigth);
        rc_si2 = getWidth(width);
        if (rc_si < OK || rc_si2 < OK)
          rc_t = GET_SIZE_FAILURE;
        sleep(1);
      }
      if (rc_t == OK) {
        destroyScreen(p->screen);
        p->screen = newScreen(*width, *heigth);
        clear();
        rc_in = initScreen(p->screen);
        pthread_mutex_unlock(&(p->mutex));

        *oldHeigth = *heigth;
        *oldWidth = *width;
      }
    }
    if (rc_t < OK || rc_in < OK || rc_si < OK || rc_si2 < OK)
      rc_t = INPUT_LOOP_FAILURE;
    usleep(50000);
  }
}

int optionsHandler(List args, const int argc, char **argv, int *n, int *m,
                   int *mode) {
  int rc_t;

  int i;
  int flagged[3] = {0, 0, 0};
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      printf("%s\n", helpMsg);
      rc_t = HELP_PRINT;
    } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--number") == 0) {
      if (!flagged[0]) {
        int rc_ca = sscanf(argv[++i], "%d", n);
        if (rc_ca == 0) {
          rc_t = CAST_FAILURE;
        }
        flagged[0] = 1;
      } else {
        printError("double n option is not allowed");
        rc_t = DOUBLE_OPTION_FAILURE;
      }
    } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--mumber") == 0) {
      if (!flagged[1]) {
        int rc_ca = sscanf(argv[++i], "%d", m);
        if (rc_ca == 0) {
          rc_t = CAST_FAILURE;
        }
        flagged[1] = 1;
      } else {
        printError("double m option is not allowed");
        rc_t = DOUBLE_OPTION_FAILURE;
      }
    } else if (strcmp(argv[i], "-f") == 0 ||
               strcmp(argv[i], "--normal-mode") == 0) {
      if (!flagged[2]) {
        *mode = NORMAL_MODE;
        flagged[2] = 1;
      } else {
        printError("double f option is not allowed");
        rc_t = DOUBLE_OPTION_FAILURE;
      }
    } else {
      // TODO find a way to handle files before options
      int rc_pu = push(args, (void *)argv[i]);
      if (rc_pu < OK) {
        rc_t = MALLOC_FAILURE;
      }
    }
  }
  return rc_t;
}

void cannelloni(void *data) {
  char *str = (char *)data;
  printf("%s\n", str);
}

int main(int argc, char **argv) {
  int rc_t = OK;
  int n = 3;
  int m = 4;
  int mode = FANCY_MODE;
  List args = newList();
  if (args == NULL)
    rc_t = MALLOC_FAILURE;

  int rc_opt = optionsHandler(args, argc, argv, &n, &m, &mode);
  rc_t = rc_opt;

  if (rc_t == OK) {
    printf("inizio il programma con n=%d e m=%d\n", n, m);
    printList(args, cannelloni);
    int *width = malloc(sizeof(int));
    int *heigth = malloc(sizeof(int));

    getHeigth(heigth);
    getWidth(width);
    if (*heigth < 28 || *width < 87) {
      printf(
          "For a better user experience we suggest you to increase the size of "
          "the terminal window. We develped a fancy graphics with a better "
          "UI\n");
      sleep(2);
      mode = NORMAL_MODE;
    }
    if (mode == FANCY_MODE) {
      pthread_t graphics, input;
      int iret1, iret2;
      set_input_mode();
      screenMutex_t screen_mutex;
      pthread_mutex_init(&screen_mutex.mutex, NULL);

      Screen screen = newScreen(*width, *heigth);

      screen_mutex.screen = screen;

      iret1 =
          pthread_create(&graphics, NULL, graphicsLoop, (void *)&screen_mutex);
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
    } else {
      printf("\033[41m \033[m\n");
    }
  } else {
    printError("Bad usage or malformed option");
    printf("%s\n", helpMsg);
  }
  return rc_t;
}
