#include <fcntl.h>
#include <limits.h>
#include <linux/limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "../config/config.h"
#include "../list/list.h"
#include "../reporter/reporter.h"
#include "../wrapping/wrapping.h"
#include "tui.h"

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
  int rc_t = SUCCESS;
  int fd[2];
  int rc_pi = createUnidirPipe(fd);
  if (rc_pi == SUCCESS) {
    int f = fork();
    if (f > 0) {
      int rc_cl, rc_cl2, rc_al, rc_re, rc_ss;
      rc_cl = closeDescriptor(fd[WRITE_CHANNEL]);
      char *dst = malloc(300 * sizeof(char));
      rc_al = checkAllocationError(dst);
      if (rc_al == SUCCESS) {
        rc_re = readDescriptor(fd[READ_CHANNEL], dst, 300);
        rc_ss = sscanf(dst, "%d", width);
        wait(NULL);
        rc_cl2 = closeDescriptor(fd[READ_CHANNEL]);
        free(dst);
        if (rc_cl < SUCCESS || rc_cl2 < SUCCESS || rc_re < SUCCESS ||
            rc_ss == 0)
          rc_t = GET_SIZE_FAILURE;
      } else {
        rc_t = MALLOC_FAILURE;
      }
    } else {
      int rc_cl, rc_cl2, rc_du;
      rc_cl = closeDescriptor(fd[READ_CHANNEL]);
      rc_du = createDup(fd[WRITE_CHANNEL], 1);
      rc_cl2 = close(fd[WRITE_CHANNEL]);
      if (rc_cl < SUCCESS || rc_cl2 < SUCCESS || rc_du < SUCCESS)
        rc_t = GET_SIZE_FAILURE;
      execlp("tput", "tput", "cols", NULL);
    }
  } else {
    rc_t = PIPE_FAILURE;
  }
  return rc_t;
}

int getHeigth(int *heigth) {
  int rc_t = SUCCESS;
  int fd[2];
  int rc_pi = createUnidirPipe(fd);
  if (rc_pi == SUCCESS) {
    int f = fork();
    if (f > 0) {
      int rc_cl, rc_cl2, rc_al, rc_re, rc_ss;
      rc_cl = closeDescriptor(fd[WRITE_CHANNEL]);
      char *dst = malloc(300 * sizeof(char));
      rc_al = checkAllocationError(dst);
      if (rc_al == SUCCESS) {
        rc_re = readDescriptor(fd[READ_CHANNEL], dst, 300);
        rc_ss = sscanf(dst, "%d", heigth);
        wait(NULL);
        rc_cl2 = closeDescriptor(fd[READ_CHANNEL]);
        free(dst);
        if (rc_cl < SUCCESS || rc_cl2 < SUCCESS || rc_re < SUCCESS ||
            rc_ss == 0)
          rc_t = GET_SIZE_FAILURE;
      } else {
        rc_t = MALLOC_FAILURE;
      }
    } else {
      int rc_cl, rc_cl2, rc_du;
      rc_cl = closeDescriptor(fd[READ_CHANNEL]);
      rc_du = createDup(fd[WRITE_CHANNEL], 1);
      rc_cl2 = close(fd[WRITE_CHANNEL]);
      if (rc_cl < SUCCESS || rc_cl2 < SUCCESS || rc_du < SUCCESS)
        rc_t = GET_SIZE_FAILURE;
      execlp("tput", "tput", "lines", NULL);
    }
  } else {
    rc_t = PIPE_FAILURE;
  }
  return rc_t;
}

int isStringEqual(void *data, void *data2) {
  char *s1 = (char *)data;
  char *s2 = (char *)data2;

  fprintf(stderr, "stringa 1 %s\n", s1);
  fprintf(stderr, "stringa 2 %s\n", s2);

  int rc_t = -1;
  if (strcmp(s1, s2) == 0) {
    rc_t = 0;
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
  int rc_t = SUCCESS;

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
  if (rc_al == SUCCESS) {
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
  if (rc_rd == SUCCESS)
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

void lastDir(char *tmpCwd, char *cwd) {
  int lastSlash = 0;
  int index = 0;
  while (cwd[index] != '\0') {
    if (cwd[index++] == '/')
      lastSlash = index;
  }
  index = 0;
  while (index < 17) {
    tmpCwd[index++] = cwd[lastSlash++];
  }
  tmpCwd[16] = '\0';
}

void drawTree(Screen screen, List directories, List files, List toggled,
              char *cwd) {
  char *tmpCwd = malloc(17 * sizeof(char));
  lastDir(tmpCwd, cwd);
  writeScreen(screen, tmpCwd, 2, 7);

  //! If the code breks, move this in another position
  char *totalPath = malloc(PATH_MAX * sizeof(char));
  char *line = malloc(17 * sizeof(char));
  int lineCounter = 10;
  Node element = directories->head;
  while (element != NULL) {
    strcpy(line, (char *)element->data);
    lastDir(tmpCwd, line);
    if (lineCounter < screen->rows - 1) {
      writeScreen(screen, "                ", 2, lineCounter);
      writeScreen(screen, tmpCwd, 2, lineCounter);
      lineCounter++;
    }
    element = element->next;
  }
  element = files->head;
  while (element != NULL) {
    strcpy(totalPath, cwd);
    strcat(totalPath, "/");
    strcat(totalPath, element->data);
    int isToggled = isIn(toggled, totalPath, isStringEqual);
    fprintf(stderr, "controllo il percorso %s %d\n", totalPath, isToggled);
    if (isToggled == SUCCESS) {
      moveCursor(2, lineCounter + 1);
      printf("\033[41m \033[m");
    }

    strcpy(line, (char *)element->data);
    lastDir(tmpCwd, line);
    if (lineCounter < screen->rows - 1) {
      writeScreen(screen, "                ", 2, lineCounter);
      writeScreen(screen, tmpCwd, 2, lineCounter);
      lineCounter++;
    }
    element = element->next;
  }
  moveCursor(screen->cols, screen->rows);
  free(totalPath);
  free(line);
  free(tmpCwd);
}

// TODO write command in tui.h if we use it
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

// TODO write command in tui.h if we use it
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
  if (rc_t == SUCCESS) {
    writeScreen(screen, "input: ", 2, 1);
    writeScreen(screen,
                " comandi: quit, n, m, tree, maiuscole, minuscole, punteg., "
                "cifre, tutto",
                1, 3);
    /* writeScreen(p->screen, command, 7, 7); */
    writeScreen(screen, " .. ", 1, 8);
    writeScreen(screen, " . ", 1, 9);
    draw(screen);
    writeScreen(screen, "LOG/ERROR", 21, screen->rows - 4);
  }

  return rc_t;
}

void *graphicsLoop(void *ptr) {
  userInput_t *p = (userInput_t *)(ptr);
  int rc_t = SUCCESS;
  int rc_pi, rc_du, rc_in;
  int rc_commandCheck = 0;
  int rc_fileCheck = 0;
  int fd[2];

  // TODO add support for this log
  rc_pi = createUnidirPipe(fd);
  /* if (rc_pi == SUCCESS) */
  /*   rc_du = dup2(fd[1], 2); */

  clear();

  pthread_mutex_lock(&(p->mutex));
  rc_in = initScreen(p->screen);
  pthread_mutex_unlock(&(p->mutex));
  moveCursor(3, 2);

  int counter = 0;
  if (rc_pi < SUCCESS || rc_du < SUCCESS || rc_in < SUCCESS)
    rc_t = GRAPHIC_LOOP_FAILURE;
  while (rc_t == SUCCESS) {
    pthread_mutex_lock(&(p->mutex));
    draw(p->screen);
    drawTree(p->screen, p->userInput->directories, p->userInput->files,
             p->userInput->results, p->cwd);
    pthread_mutex_unlock(&(p->mutex));
    usleep(1000000);
    /* usleep(10000000); */
  }
  kill(getpid(), SIGKILL);
}

void drawInputLine(Screen screen) {
  int i = 0;
  moveCursor(10, 2);
  for (i = 9; i < screen->cols - 1; i++) {
    printf("%c", screen->grid[i][1]);
  }
  /* moveCursor(3, 6); */
  /* for (i = 2; i < 18; i++) { */
  /*   printf("%c", screen->grid[i][5]); */
  /* } */
  int c, r;
  for (r = 5; r < screen->rows - 1; r++) {
    moveCursor(3, r + 1);
    for (c = 2; c < 18; c++)
      printf("%c", screen->grid[c][r]);
  }
  fflush(stdout);
}

void updateTable(Screen screen, unsigned long long *table) {
  int rc_t = SUCCESS;
  char *msg = malloc(300 * sizeof(char));

  int rc_al = checkAllocationError(msg);
  if (rc_al < SUCCESS)
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
      /* int rndChoose = rand() % 2; */
      /* int rnd; */
      /* if (rndChoose == 0) */
      /*   rnd = rand() % 1000; */
      /* else */
      /*   rnd = rand() % 100; */
      unsigned long long count = table[i];
      sprintf(msg, "%c", i);
      writeScreen(screen, msg, col, row);
      sprintf(msg, "%llu", count);
      if (count >= 1000000)
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

void trim(char *string) {
  int len = strlen(string) - 1;
  while (string[len] == ' ') {
    /* printf("%d %c\n", string[len], string[len]); */
    len--;
  }
  string[len + 1] = '\0';
}

void *inputLoop(void *ptr) {
  userInput_t *p = (userInput_t *)(ptr);
  int rc_t = SUCCESS;
  int rc_al, rc_al2, rc_al3, rc_al4, rc_si, rc_si2, rc_in = SUCCESS;
  int *oldWidth = malloc(sizeof(int));
  rc_al = checkAllocationError(oldWidth);
  int *oldHeigth = malloc(sizeof(int));
  rc_al2 = checkAllocationError(oldHeigth);

  int *width = malloc(sizeof(int));
  rc_al3 = checkAllocationError(width);
  int *heigth = malloc(sizeof(int));
  rc_al4 = checkAllocationError(heigth);
  if (rc_al == SUCCESS && rc_al2 == SUCCESS && rc_al3 == SUCCESS &&
      rc_al4 == SUCCESS) {
    rc_si = getHeigth(heigth);
    rc_si2 = getWidth(width);
    *oldHeigth = *heigth;
    *oldWidth = *width;
    if (rc_si < SUCCESS || rc_si2 < SUCCESS)
      rc_t = GET_SIZE_FAILURE;
  } else
    rc_t = MALLOC_FAILURE;

  pthread_mutex_lock(&(p->mutex));
  int column = 9;
  int row = 1;
  p->screen->grid[column][1] = '|';
  pthread_mutex_unlock(&(p->mutex));

  char key;
  char lastKey;
  int treeMode = 0;
  int numberManagerMode = 0;
  int numberWorkerMode = 0;
  while (rc_t == SUCCESS) {
    key = EOF;
    pthread_mutex_lock(&(p->mutex));
    updateTable(p->screen, p->userInput->table);
    pthread_mutex_unlock(&(p->mutex));

    pthread_mutex_lock(&(p->mutex));
    key = getkey();
    if ((key > 31 && key <= 127) || key == 10 || key == 27) {
      if (key == 10) {
        int i;
        int cmdCounter = 0;
        char *cmd = malloc(PATH_MAX * sizeof(char));
        for (i = 9; i < p->screen->cols - 1; i++) {
          if (p->screen->grid[i][1] != '|') {
            cmd[cmdCounter++] = p->screen->grid[i][1];
          }
          if (treeMode == 0) {
            p->screen->grid[i][1] = ' ';
          }
        }
        cmd[cmdCounter] = '\0';
        trim(cmd);
        /* p->screen->cmd = 4; */

        if (strcmp(cmd, "maiuscole") == 0) {
          writeScreen(p->screen, "input: ", 2, 1);
          p->screen->cmd = 1;
          row = 1;
          column = 9;
          free(cmd);
        } else if (strcmp(cmd, "minuscole") == 0) {
          writeScreen(p->screen, "input: ", 2, 1);
          p->screen->cmd = 2;
          row = 1;
          column = 9;
          free(cmd);
        } else if (strcmp(cmd, "punteg.") == 0) {
          writeScreen(p->screen, "input: ", 2, 1);
          p->screen->cmd = 3;
          row = 1;
          column = 9;
          free(cmd);
        } else if (strcmp(cmd, "cifre") == 0) {
          writeScreen(p->screen, "input: ", 2, 1);
          p->screen->cmd = 4;
          row = 1;
          column = 9;
          free(cmd);
        } else if (strcmp(cmd, "tutto") == 0) {
          writeScreen(p->screen, "input: ", 2, 1);
          p->screen->cmd = 5;
          row = 1;
          column = 9;
          free(cmd);
        } else if (strcmp(cmd, "tree") == 0) {
          writeScreen(p->screen, "tree:  press esc to return in input mode", 2,
                      1);
          row = 5;
          column = 2;
          treeMode = 1;
          free(cmd);
        } else if (strcmp(cmd, "quit") == 0) {
          clear();
          moveCursor(0, 0);
          free(cmd);
          exit(0);
        } else if (strcmp(cmd, "n") == 0) {
          writeScreen(p->screen, "n    : ", 2, 1);
          row = 1;
          column = 9;
          free(cmd);
          numberWorkerMode = 1;
        } else if (strcmp(cmd, "m") == 0) {
          writeScreen(p->screen, "m    : ", 2, 1);
          row = 1;
          column = 9;
          free(cmd);
          numberManagerMode = 1;
        } else {
          if (numberManagerMode == 1) {
            writeScreen(p->screen, "input: ", 2, 1);
            int castPlaceholder;
            sscanf(cmd, "%d", &castPlaceholder);
            p->userInput->managers = castPlaceholder;
            numberManagerMode = 0;
            free(cmd);
            row = 1;
            column = 9;
          } else if (numberWorkerMode == 1) {
            writeScreen(p->screen, "input: ", 2, 1);
            int castPlaceholder;
            sscanf(cmd, "%d", &castPlaceholder);
            p->userInput->workers = castPlaceholder;
            numberWorkerMode = 0;
            free(cmd);
            row = 1;
            column = 9;
          } else if (treeMode == 1) {
            writeScreen(p->screen, "input: ", 2, 1);
            free(cmd);
            char *tree = malloc(PATH_MAX * sizeof(char));
            cmdCounter = 0;
            for (i = 2; i < 18; i++) {
              if (p->screen->grid[i][5] != '|') {
                tree[cmdCounter++] = p->screen->grid[i][5];
              }
              p->screen->grid[i][5] = ' ';
            }
            tree[cmdCounter] = '\0';
            trim(tree);
            if (strcmp(tree, "..") == 0) {
              free(p->userInput->tree);
              p->userInput->tree = tree;
            } else if (strcmp(tree, ".") == 0) {
              Node element = p->userInput->files->head;
              while (element != NULL) {
                char *totalPath = malloc(PATH_MAX * sizeof(char));
                char *f = malloc(PATH_MAX * sizeof(char));
                strcpy(f, element->data);
                strcpy(totalPath, p->cwd);
                strcat(totalPath, "/");
                strcat(totalPath, f);
                int isToggled = deleteNode(p->userInput->results, totalPath,
                                           isStringEqual, free);
                if (isToggled != SUCCESS) {
                  fprintf(stderr, "faccio il toggle %s\n", totalPath);
                  push(p->userInput->results, totalPath);
                } else {
                  free(totalPath);
                }
                free(f);
                element = element->next;
              }
              // TODO ask to other if also directories must have the same
              // behavior
            } else {
              int isDirectory =
                  isIn(p->userInput->directories, tree, isStringEqual);
              if (isDirectory == SUCCESS) {
                char *cwd = malloc(PATH_MAX * sizeof(char));
                strcpy(cwd, p->cwd);
                strcat(cwd, "/");
                strcat(cwd, tree);
                int index = 0;
                // TODO control overflow
                while (cwd[index] != '\0') {
                  int old = index;
                  index++;
                  cwd[old] = cwd[index];
                }
                p->userInput->tree = cwd;
              } else {
                int isFile = isIn(p->userInput->files, tree, isStringEqual);

                if (isFile == SUCCESS) {
                  char *cwd = malloc(PATH_MAX * sizeof(char));
                  strcpy(cwd, p->cwd);
                  strcat(cwd, "/");
                  strcat(cwd, tree);
                  int rc_re =
                      removeNode(p->userInput->results, cwd, isStringEqual);
                  if (rc_re != SUCCESS)
                    push(p->userInput->results, cwd);
                }
                free(tree);
              }
            }
            row = 5;
            column = 2;
          } else if (strcmp(cmd, "") != 0) {
            writeScreen(p->screen, "input: ", 2, 1);
            /* printf("sto incodando come un caimano %lu\n", strlen(cmd)); */
            enqueue(p->userInput->paths, cmd);
            row = 1;
            column = 9;
          }
        }

        p->screen->grid[column][row] = '|';
      } else if (key == 27) {
        if (treeMode) {
          int i;
          for (i = 2; i < 18; i++) {
            p->screen->grid[i][row] = ' ';
          }
          row = 1;
          column = 9;
          treeMode = 0;
          writeScreen(p->screen, "input: ", 2, 1);
          for (i = 9; i < p->screen->cols - 1; i++) {
            p->screen->grid[i][row] = ' ';
          }
        }
      } else if (key == 127) {
        if ((column > 9 && !treeMode) || (treeMode && column > 2)) {
          p->screen->grid[column--][row] = ' ';
          p->screen->grid[column][row] = '|';
        }
      } else if (key == '[' || key == ']') {
        lastKey = key;
      } else if (!((key == 65 || key == 66 || key == 67 || key == 68) &&
                   lastKey == '[')) {
        if (numberManagerMode == 0 && numberWorkerMode == 0) {
          if ((column < p->screen->cols - 2 && !treeMode) ||
              (treeMode && column < 18)) {
            p->screen->grid[column++][row] = key;
            p->screen->grid[column][row] = '|';
          }
        } else {
          if ((column < p->screen->cols - 2 && !treeMode) ||
              (treeMode && column < 18)) {
            if (key >= 48 && key <= 57) {
              p->screen->grid[column++][row] = key;
              p->screen->grid[column][row] = '|';
            }
          }
        }
      }
      drawInputLine(p->screen);
      lastKey = key;
    }
    pthread_mutex_unlock(&(p->mutex));

    rc_si = getHeigth(heigth);
    rc_si2 = getWidth(width);
    if (rc_si < SUCCESS || rc_si2 < SUCCESS)
      rc_t = GET_SIZE_FAILURE;
    if ((*oldHeigth != *heigth || *oldWidth != *width) && rc_t == SUCCESS) {
      pthread_mutex_lock(&(p->mutex));
      while ((*heigth < 28 || *width < 87) && rc_t == SUCCESS) {
        clear();
        printf(
            "window dimension is to small, please increase window size or "
            "luanch programma with -f option to use without fancy graphic at "
            "any size\n");
        rc_si = getHeigth(heigth);
        rc_si2 = getWidth(width);
        if (rc_si < SUCCESS || rc_si2 < SUCCESS)
          rc_t = GET_SIZE_FAILURE;
        sleep(1);
      }
      if (rc_t == SUCCESS) {
        destroyScreen(p->screen);
        p->screen = newScreen(*width, *heigth);
        clear();
        rc_in = initScreen(p->screen);
        pthread_mutex_unlock(&(p->mutex));

        *oldHeigth = *heigth;
        *oldWidth = *width;
      }
    }
    if (rc_t < SUCCESS || rc_in < SUCCESS || rc_si < SUCCESS ||
        rc_si2 < SUCCESS)
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
      if (rc_pu < SUCCESS) {
        rc_t = MALLOC_FAILURE;
      }
    }
  }
  return rc_t;
}
