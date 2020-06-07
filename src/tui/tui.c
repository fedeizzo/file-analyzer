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
    "Usage: counter [OPTIONS] [FILES] [FOLDERS]\n\n"
    "OPTIONS:\n"
    "    -h, --help        : print this message\n"
    "    -n, --number      : specify the workers amount\n"
    "    -m, --mumber      : specify the managers amount\n"
    "    -f, --normal-mode : normal mode without fancy graphic\n";

char *guiHelpMsg[] = {"Welcome, this is a list of command:",
                      "  quit      -> quit programm",
                      "  help      -> display this message",
                      "  input     -> write a path (absolute or relative)",
                      "  n         -> change worker amount",
                      "  m         -> change manager amount",
                      "  tree      -> enter tree mode",
                      "    * up, down arrow -> to navigate",
                      "    * .              -> to toggle/untoggle all",
                      "    * ..             -> to navigate father directory",
                      "  maiuscole -> display only maiuscole",
                      "  minuscole -> display only minuscole",
                      "  punteg.   -> display only punteg.",
                      "  cifre     -> display only cifre",
                      "  tutto     -> display all"};
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
  int rc_al = checkAllocationError(screen);
  if (rc_al == SUCCESS) {
    screen->rows = heigth;
    screen->cols = width;
    screen->cmd = 5;
    screen->treeStartCol = 0;
    screen->treeEndCol = 17;
    screen->grid = malloc(screen->cols * sizeof(char *));
    int rc_al2 = checkAllocationError(screen->grid);
    if (rc_al2 == SUCCESS) {
      int i = 0;
      int rc_for = SUCCESS;
      for (i = 0; i < screen->cols && rc_for == SUCCESS; i++) {
        screen->grid[i] = malloc(heigth * sizeof(char));
        rc_for = checkAllocationError(screen->grid[i]);
      }
    }
  }

  return screen;
}

void destroyScreen(Screen screen) {
  int i = 0;
  for (i = 0; i < screen->cols; i++) {
    if (screen->grid[i] != NULL)
      free(screen->grid[i]);
  }

  if (screen->grid != NULL)
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
      kill(getpid(), SIGKILL);
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
      kill(getpid(), SIGKILL);
    }
  } else {
    rc_t = PIPE_FAILURE;
  }
  return rc_t;
}

int isStringEqual(void *data, void *data2) {
  char *s1 = (char *)data;
  char *s2 = (char *)data2;

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
               (r == 8 && 0 < c && c < 19) ||
               (r == screen->rows - 5 && 19 < c && c < screen->cols - 1) ||
               r == screen->rows - 1)
        screen->grid[c][r] = HOR_TABLE;
      else if (c == 0 || (c == 19 && r != 1) || c == screen->cols - 1)
        screen->grid[c][r] = VER_TABLE;
      else
        screen->grid[c][r] = ' ';
    }
  }

  return rc_t;
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
    if (((counter >= 33 && counter <= 47)) ||
        ((counter >= 58 && counter <= 64)) ||
        ((counter >= 91 && counter <= 96)) ||
        ((counter >= 123 && counter <= 126)))
      rc_t = 0;
  } else if (cmd == 4) {
    if (counter >= 48 && counter <= 57)
      rc_t = 0;
  } else if (cmd == 5) {
    if (counter >= 0 && counter <= 128)
      rc_t = 0;
  } else if (cmd == 6) {
    if ((counter >= 0 && counter <= 32) || counter == 127 || counter == 128)
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

void lastDir(char *tmpCwd, char *cwd, int *start, int *end) {
  int lastSlash = 0;
  int index = 0;
  while (cwd[index] != '\0') {
    if (cwd[index++] == '/')
      lastSlash = index;
  }
  int until = 17;
  int str = *start;
  if (strlen(cwd) - lastSlash > 17) {
    int nextStart = (lastSlash + str) % (strlen(cwd) - lastSlash);
    lastSlash += nextStart;
  }
  index = 0;
  while (index < until && cwd[lastSlash] != '\0') {
    tmpCwd[index++] = cwd[lastSlash++];
  }
  tmpCwd[index] = '\0';
}

void drawTree(Screen screen, List directories, List files, List toggled,
              char *cwd, int *startCol, int *endCol, const int startRow,
              const int endRow) {
  char *tmpCwd = malloc(17 * sizeof(char));
  char *line = malloc(17 * sizeof(char));
  int rc_al = checkAllocationError(tmpCwd);
  int rc_al2 = checkAllocationError(tmpCwd);

  if (rc_al == SUCCESS && rc_al2 == SUCCESS) {
    if (strcmp(cwd, "/") == 0) {
      strcpy(tmpCwd, cwd);
    } else {
      lastDir(tmpCwd, cwd, startCol, endCol);
    }
    writeScreen(screen, "                 ", 2, 7);
    writeScreen(screen, tmpCwd, 2, 7);

    int lineCounter = 11;
    Node element = directories->head;
    int elementPrinted = 0;
    while (element != NULL) {
      strcpy(line, (char *)element->data);
      lastDir(tmpCwd, line, startCol, endCol);
      if (lineCounter < screen->rows - 1 && elementPrinted >= startRow) {
        writeScreen(screen, "                 ", 2, lineCounter);
        writeScreen(screen, tmpCwd, 2, lineCounter);
        lineCounter++;
      }
      element = element->next;
      elementPrinted++;
    }
    element = files->head;
    while (element != NULL) {
      int isToggled = isIn(toggled, element->data, isStringEqual);
      if (isToggled == SUCCESS && lineCounter < screen->rows - 1 &&
          elementPrinted >= startRow) {
        moveCursor(2, lineCounter + 1);
        printf("\033[41m \033[m");
      }

      strcpy(line, (char *)element->data);
      lastDir(tmpCwd, line, startCol, endCol);
      if (lineCounter < screen->rows - 1 && elementPrinted >= startRow) {
        writeScreen(screen, "                 ", 2, lineCounter);
        writeScreen(screen, tmpCwd, 2, lineCounter);
        lineCounter++;
      }
      element = element->next;
      elementPrinted++;
    }

    while (lineCounter < screen->rows - 1) {
      writeScreen(screen, "                 ", 2, lineCounter);
      lineCounter++;
    }

    moveCursor(screen->cols, screen->rows);
    free(line);
    free(tmpCwd);
  }
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
    writeScreen(
        screen,
        " comandi: quit, help, n, m, tree, maiuscole, minuscole, punteg., "
        "cifre, tutto",
        1, 3);

    writeScreen(screen, " .. ", 1, 9);
    writeScreen(screen, " . ", 1, 10);
    draw(screen);
    writeScreen(screen, "STATISTICS", 21, screen->rows - 4);
  }

  return rc_t;
}

void computeStatistics(Screen screen, unsigned long long *table) {
  char clearString[screen->cols - 22];
  int i, j = 0;
  for (i = 0; i < screen->cols - 22; i++) {
    clearString[i] = ' ';
  }
  clearString[screen->cols - 23] = '\0';
  writeScreen(screen, clearString, 21, screen->rows - 3);

  unsigned long long tot = 0;
  unsigned long long selected = 0;
  double percentageMaiuscole = 0;
  double percentageMinuscole = 0;
  double percentagePunteg = 0;
  double percentageCifre = 0;
  double percentageOther = 0;
  double percentageTotal = 100;
  for (j = 1; j < 7; j++) {
    selected = 0;
    tot = 0;
    for (i = 0; i < NCHAR_TABLE; i++) {
      tot += table[i];
      if (commandFilter(j, i) == 0) {
        selected += table[i];
      }
    }
    double percentage = 0;
    if (tot != 0 && selected != 0) {
      percentage = (double)((long double)selected / (long double)tot * 100);
    }
    switch (j) {
    case 1:
      percentageMaiuscole = percentage;
      break;
    case 2:
      percentageMinuscole = percentage;
      break;
    case 3:
      percentagePunteg = percentage;
      break;
    case 4:
      percentageCifre = percentage;
      break;
    case 6:
      percentageOther = percentage;
      break;
    }
  }
  char tmpString[100];
  /* sprintf(tmpString, "Percentage over total: %.2f%%", percentage); */
  sprintf(tmpString,
          "Total    : %6.2f%%   Cifre  : %6.2f%%   Minuscole: %6.2f%%",
          percentageTotal, percentageCifre, percentageMinuscole);
  writeScreen(screen, tmpString, 21, screen->rows - 3);
  sprintf(tmpString,
          "Maiuscole: %6.2f%%   Punteg.: %6.2f%%   Other    : %6.2f%%",
          percentageMaiuscole, percentagePunteg, percentageOther);
  writeScreen(screen, tmpString, 21, screen->rows - 2);
}

void *graphicsLoop(void *ptr) {
  userInput_t *p = (userInput_t *)(ptr);
  int rc_t = SUCCESS;
  int rc_pi, rc_du, rc_in;
  int rc_commandCheck = 0;
  int rc_fileCheck = 0;
  int fd[2];

  clear();

  pthread_mutex_lock(&(p->mutex));
  rc_in = initScreen(p->screen);
  pthread_mutex_unlock(&(p->mutex));
  moveCursor(3, 2);

  int counter = 0;
  int scrollCounte = 0;
  if (rc_pi < SUCCESS || rc_du < SUCCESS || rc_in < SUCCESS)
    rc_t = GRAPHIC_LOOP_FAILURE;
  while (rc_t == SUCCESS) {
    pthread_mutex_lock(&(p->mutex));
    computeStatistics(p->screen, p->userInput->table);
    draw(p->screen);
    drawTree(p->screen, p->userInput->directories, p->userInput->files,
             p->userInput->results, p->cwd, &p->screen->treeStartCol,
             &p->screen->treeEndCol, p->screen->treeStartRow, 0);
    pthread_mutex_unlock(&(p->mutex));
    usleep(650000);
    if (scrollCounte % 2 == 0) {
      if (p->screen->treeStartCol >= PATH_MAX) {
        p->screen->treeEndCol = 0;
        p->screen->treeStartCol = 0;
      } else {
        p->screen->treeEndCol++;
        p->screen->treeStartCol++;
      }
    }
  }
  kill(getpid(), SIGKILL);
}

void drawInputLine(Screen screen) {
  int i = 0;
  moveCursor(10, 2);
  for (i = 9; i < screen->cols - 1; i++) {
    printf("%c", screen->grid[i][1]);
  }
  int c, r;
  for (r = 5; r < screen->rows - 1; r++) {
    moveCursor(3, r + 1);
    for (c = 2; c < 18; c++)
      printf("%c", screen->grid[c][r]);
  }
  fflush(stdout);
}

void clearCenter(Screen screen) {
  int i = 0;
  int j = 0;
  for (i = 21; i < screen->cols - 1; i++) {
    for (j = 5; j < screen->rows - 5; j++) {
      screen->grid[i][j] = ' ';
    }
  }
}

void updateTable(Screen screen, unsigned long long *table) {
  int rc_t = SUCCESS;

  int i = 0;
  int col = 21;
  int row = 7;
  char *msg = malloc(PATH_MAX * sizeof(char));
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

    i = 0;
    col = 22;
    row = 7;
    for (i = 33; i < 127; i++) {
      sprintf(msg, "         ");
      writeScreen(screen, msg, col + 2, row);
      if (commandFilter(screen->cmd, i) != 0) {
        sprintf(msg, " ");
        writeScreen(screen, msg, col, row);
        sprintf(msg, "         ");
        writeScreen(screen, msg, col + 2, row++);
      } else {
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
    sprintf(msg, "other");
    if (row >= screen->rows - 5) {
      row = 7;
      col += 11;
    }
    writeScreen(screen, msg, col, row++);
    if (row >= screen->rows - 5) {
      row = 7;
      col += 11;
    }
    sprintf(msg, "         ");
    writeScreen(screen, msg, col, row);

    unsigned long long count = 0;
    for (i = 0; i < 33; i++) {
      count += table[i];
    }
    count += table[NCHAR_TABLE - 1];
    count += table[NCHAR_TABLE - 2];
    sprintf(msg, "%llu", count);
    if (commandFilter(screen->cmd, 128) == 0) {
      if (count >= 1000000)
        writeScreen(screen, msg, col, row++);
      else
        writeScreen(screen, msg, col + 1, row++);
    }

    free(msg);
  } else {
    rc_t = MALLOC_FAILURE;
  }
}

void drawHelpMsg(Screen screen) {
  int i = 0;
  int col = 21;
  int row = 7;
  int start = (((screen->rows - 5) - row) - 15) / 2;
  int end = (((screen->cols - 1) - col) - 52) / 2;
  row += start;
  col += end;
  for (i = 0; i < 15; i++) {
    writeScreen(screen, guiHelpMsg[i], col, row++);
  }
}

void drawCenter(Screen screen, unsigned long long *table) {
  clearCenter(screen);
  if (screen->cmd == 6)
    drawHelpMsg(screen);
  else
    updateTable(screen, table);
}

void trim(char *string) {
  int len = strlen(string) - 1;
  while (string[len] == ' ') {
    len--;
  }
  string[len + 1] = '\0';
}

int resize(int *oldHeigth, int *oldWidth, int *heigth, int *width) {
  int rc_si = getHeigth(heigth);
  int rc_si2 = getWidth(width);
  int rc_t = SUCCESS;
  int rc_in = SUCCESS;

  if (rc_si < SUCCESS || rc_si2 < SUCCESS)
    rc_t = GET_SIZE_FAILURE;
  if ((*oldHeigth != *heigth || *oldWidth != *width) && rc_t == SUCCESS) {
    while ((*heigth < 28 || *width < 89) && rc_t == SUCCESS) {
      clear();
      printf("window dimension is to small, please increase window size or "
             "luanch programma with -f option to use without fancy graphic at "
             "any size\n");
      rc_si = getHeigth(heigth);
      rc_si2 = getWidth(width);
      if (rc_si < SUCCESS || rc_si2 < SUCCESS)
        rc_t = GET_SIZE_FAILURE;
      sleep(1);
    }
  }

  if (rc_t < SUCCESS || rc_in < SUCCESS || rc_si < SUCCESS || rc_si2 < SUCCESS)
    rc_t = INPUT_LOOP_FAILURE;
  return rc_t;
}

void changeCommandMode(Screen screen, int *cmd, int cmdMode, int *row,
                       int *column) {
  writeScreen(screen, "input: ", 2, 1);
  *cmd = cmdMode;
  *row = 1;
  *column = 9;
}

/**
 * Moves backward in file system tree
 *
 * args:
 *    Screen screen      : the screen where some infos are printed
 *    UserInput userInput: user input
 *    char *cwd          : current location in file system tree
 */
void moveBackward(Screen screen, UserInput userInput, char *cwd) {
  userInput->tree[0] = '\0';
  if (strcmp(cwd, "/") != 0) {
    char *tmpPath = malloc(sizeof(char) * PATH_MAX);
    if (checkAllocationError(tmpPath) == SUCCESS) {
      chdir("..");
      getcwd(cwd, PATH_MAX);
      strcpy(userInput->tree, cwd);
      free(tmpPath);
    }
  }
  screen->treeStartCol = 0;
  screen->treeEndCol = 0;
  screen->treeStartRow = 0;
}

/**
 * Toggles all files in the current directory
 *
 * args:
 *    UserInput userInput: userInput
 */
void toggleAll(UserInput userInput) {
  userInput->toggledChanged = 1;
  Node element = userInput->files->head;
  while (element != NULL) {
    char *totalPath = malloc(PATH_MAX * sizeof(char));
    if (checkAllocationError(totalPath) == SUCCESS) {
      strcpy(totalPath, element->data);
      int isToggled =
          deleteNode(userInput->results, totalPath, isStringEqual, free);
      if (isToggled != SUCCESS) {
        push(userInput->results, totalPath);
      } else {
        free(totalPath);
      }
      element = element->next;
    }
  }
  int i = 0;
  for (i = 0; i < NCHAR_TABLE; i++) {
    userInput->table[i] = 0;
  }
}

void changeComponentAmount(Screen screen, char *cmd, int *componentAmount,
                           int *row, int *column) {
  writeScreen(screen, "input: ", 2, 1);
  int castPlaceholder;
  if (strcmp(cmd, "") != 0) {
    sscanf(cmd, "%d", &castPlaceholder);
    *componentAmount = castPlaceholder;
  }
  *row = 1;
  *column = 9;
}

/**
 * Selects a node for which we want to know the children
 *
 * args:
 *    Screen screen      : the screen where some infos are printed
 *    UserInput userInput: user input
 *    char *cwd          : current location in file system tree
 *    char *realCwd      : current real location in file system tree
 */
void selectNode(Screen screen, UserInput userInput, char *realCwd, char *tree) {
  char *cwd = malloc(PATH_MAX * sizeof(char));
  if (checkAllocationError(cwd) == SUCCESS) {
    strcpy(cwd, realCwd);
    if (strcmp(realCwd, "/") != 0) {
      strcat(cwd, "/");
    }
    strcat(cwd, tree);

    int isDirectory = isIn(userInput->directories, cwd, isStringEqual);
    if (isDirectory == SUCCESS) {
      int index = 0;
      screen->treeStartCol = 0;
      screen->treeEndCol = 0;
      screen->treeStartRow = 0;
      strcpy(userInput->tree, cwd);
      strcpy(realCwd, cwd);
      free(cwd);
    } else {
      int isFile = isIn(userInput->files, cwd, isStringEqual);

      if (isFile == SUCCESS) {
        int rc_re = deleteNode(userInput->results, cwd, isStringEqual, free);
        if (rc_re != SUCCESS) {
          push(userInput->results, cwd);
        } else {
          int i = 0;
          for (i = 0; i < NCHAR_TABLE; i++) {
            userInput->table[i] = 0;
          }
        }
      }
    }
  }
  free(tree);
}

/**
 * Adds a file in List that contains all files that must be sent to the analyzer
 *
 * args:
 *    Screen screen      : the screen where some infos are printed
 *    UserInput userInput: user input
 *    char *cwd          : the current location in file system tree
 *    char *cmd          : the user input
 *    int *row           : the row where the cursor is moved after the update
 *    int *column        : the column where the cursor is moved after update
 */
void askComputation(Screen screen, UserInput userInput, char *cwd, char *cmd,
                    int *row, int *column) {
  writeScreen(screen, "input: ", 2, 1);
  char *path = malloc(PATH_MAX * sizeof(char));
  char *rlPath = malloc(PATH_MAX * sizeof(char));
  if (checkAllocationError(path) == SUCCESS &&
      checkAllocationError(rlPath) == SUCCESS) {
    if (cmd[0] != '/') {
      strcpy(path, cwd);
      strcat(path, "/");
      strcat(path, cmd);
      realpath(path, rlPath);
    } else {
      strcpy(rlPath, cmd);
    }
    free(path);
    enqueue(userInput->paths, rlPath);
  }
  *row = 1;
  *column = 9;
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
  int isTreeLastMode = 0;
  int treeMode = 0;
  int numberManagerMode = 0;
  int numberWorkerMode = 0;
  char *treeInput = malloc(PATH_MAX * sizeof(char));
  int rc_al5 = checkAllocationError(treeInput);
  if (rc_al5 != SUCCESS)
    rc_t = MALLOC_FAILURE;
  int counterTreeInput = 0;
  while (rc_t == SUCCESS) {
    key = EOF;
    pthread_mutex_lock(&(p->mutex));
    drawCenter(p->screen, p->userInput->table);
    pthread_mutex_unlock(&(p->mutex));

    pthread_mutex_lock(&(p->mutex));
    key = getkey();
    if ((key > 31 && key <= 127) || key == 10 || key == 27) {
      if (key == 10) {
        isTreeLastMode = 0;
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

        if (strcmp(cmd, "maiuscole") == 0) {
          changeCommandMode(p->screen, &p->screen->cmd, 1, &row, &column);
          free(cmd);
        } else if (strcmp(cmd, "minuscole") == 0) {
          changeCommandMode(p->screen, &p->screen->cmd, 2, &row, &column);
          free(cmd);
        } else if (strcmp(cmd, "punteg.") == 0) {
          changeCommandMode(p->screen, &p->screen->cmd, 3, &row, &column);
          free(cmd);
        } else if (strcmp(cmd, "cifre") == 0) {
          changeCommandMode(p->screen, &p->screen->cmd, 4, &row, &column);
          free(cmd);
        } else if (strcmp(cmd, "tutto") == 0) {
          changeCommandMode(p->screen, &p->screen->cmd, 5, &row, &column);
          free(cmd);
        } else if (strcmp(cmd, "help") == 0) {
          changeCommandMode(p->screen, &p->screen->cmd, 6, &row, &column);
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
            changeComponentAmount(p->screen, cmd, &p->userInput->managers, &row,
                                  &column);
            numberManagerMode = 0;
            free(cmd);
          } else if (numberWorkerMode == 1) {
            changeComponentAmount(p->screen, cmd, &p->userInput->workers, &row,
                                  &column);
            numberWorkerMode = 0;
            free(cmd);
          } else if (treeMode == 1) {
            // READING AND PARSING INPUT
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
            p->screen->grid[18][5] = ' ';
            tree[cmdCounter] = '\0';
            trim(tree);
            treeInput[counterTreeInput] = '\0';
            strcat(treeInput, tree);
            strcpy(tree, treeInput);
            treeInput[0] = '\0';
            counterTreeInput = 0;

            if (strcmp(tree, "..") == 0) {
              moveBackward(p->screen, p->userInput, p->cwd);
            } else if (strcmp(tree, ".") == 0 &&
                       p->userInput->files->size > 0) {
              toggleAll(p->userInput);
            } else {
              selectNode(p->screen, p->userInput, p->cwd, tree);
            }
            row = 5;
            column = 2;
          } else if (strcmp(cmd, "") != 0) {
            askComputation(p->screen, p->userInput, p->cwd, cmd, &row, &column);
          }
        }

        p->screen->grid[column][row] = '|';
      } else if (key == 27) {
        isTreeLastMode = 0;
        if (treeMode) {
          treeInput[0] = '\0';
          counterTreeInput = 0;
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
          p->screen->grid[9][row] = '|';
          isTreeLastMode = 1;
        }
      } else if (key == 127) {
        isTreeLastMode = 0;
        if (treeMode && counterTreeInput > 0) {
          int i = 17;
          while (i > 1) {
            int j = i - 1;
            if (i == 2)
              p->screen->grid[i][row] = treeInput[--counterTreeInput];
            else
              p->screen->grid[i][row] = p->screen->grid[j][row];
            i = j;
          }
        } else {
          if ((column > 9 && !treeMode) || (treeMode && column > 2)) {
            p->screen->grid[column--][row] = ' ';
            p->screen->grid[column][row] = '|';
          }
        }
      } else if (key == '[' || key == ']') {
        if (key == '[' && lastKey == 27 && isTreeLastMode == 1) {
          writeScreen(p->screen, "tree:  press esc to return in input mode", 2,
                      1);
          row = 5;
          column = 2;
          treeMode = 1;
          isTreeLastMode = 0;
        }
        lastKey = key;
      } else if (!((key == 65 || key == 66 || key == 67 || key == 68) &&
                   lastKey == '[')) {
        isTreeLastMode = 0;
        if (numberManagerMode == 0 && numberWorkerMode == 0) {
          if ((column < p->screen->cols - 2 && !treeMode) ||
              (treeMode && column < 18)) {
            p->screen->grid[column++][row] = key;
            p->screen->grid[column][row] = '|';
          } else if (treeMode && column >= 18) {
            treeInput[counterTreeInput++] = p->screen->grid[2][row];
            int i = 2;
            while (p->screen->grid[i][row] != '|' || i < 16) {
              int j = i + 1;
              if (p->screen->grid[j][row] == '|')
                p->screen->grid[i][row] = key;
              else
                p->screen->grid[i][row] = p->screen->grid[j][row];
              i = j;
            }
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
      } else if (lastKey == '[' && (key == 65 || key == 66)) {
        if (treeMode == 1) {
          if (key == 65) { // UP
            p->screen->treeStartRow--;
            if (p->screen->treeStartRow < 0) {
              p->screen->treeStartRow = 0;
            }
          } else if (key == 66) { // DOWN
            p->screen->treeStartRow++;
            int max =
                p->userInput->directories->size + p->userInput->files->size - 1;
            if (p->screen->treeStartRow > max) {
              p->screen->treeStartRow--;
            }
          }
        }
      }
      drawInputLine(p->screen);
      lastKey = key;
    }
    pthread_mutex_unlock(&(p->mutex));

    pthread_mutex_lock(&(p->mutex));
    rc_t = resize(oldHeigth, oldWidth, heigth, width);
    if (rc_t == SUCCESS && (*oldHeigth != *heigth || *oldWidth != *width)) {
      destroyScreen(p->screen);
      p->screen = newScreen(*width, *heigth);
      clear();
      rc_in = initScreen(p->screen);

      *oldHeigth = *heigth;
      *oldWidth = *width;
    }
    pthread_mutex_unlock(&(p->mutex));

    usleep(50000);
  }
}

int optionsHandler(List args, const char *cwd, const int argc, char **argv,
                   int *n, int *m, int *mode) {
  int rc_t = SUCCESS;

  int i;
  int flagged[3] = {0, 0, 0};
  for (i = 1; i < argc; i++) {
    if ((strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) &&
        args->size == 0) {
      printf("%s\n", helpMsg);
      rc_t = HELP_PRINT;
    } else if ((strcmp(argv[i], "-n") == 0 ||
                strcmp(argv[i], "--number") == 0) &&
               args->size == 0) {
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
    } else if ((strcmp(argv[i], "-m") == 0 ||
                strcmp(argv[i], "--mumber") == 0) &&
               args->size == 0) {
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
    } else if ((strcmp(argv[i], "-f") == 0 ||
                strcmp(argv[i], "--normal-mode") == 0) &&
               args->size == 0) {
      if (!flagged[2]) {
        *mode = NORMAL_MODE;
        flagged[2] = 1;
      } else {
        printError("double f option is not allowed");
        rc_t = DOUBLE_OPTION_FAILURE;
      }
    } else {
      char *newAbsolutePath = malloc(PATH_MAX * sizeof(char));
      int rc_al = checkAllocationError(newAbsolutePath);
      if (rc_al == SUCCESS) {
        if (argv[i][0] != '/') {
          strcpy(newAbsolutePath, cwd);
          strcat(newAbsolutePath, "/");
          strcat(newAbsolutePath, argv[i]);
        } else {
          strcpy(newAbsolutePath, argv[i]);
        }
        int rc_pu = enqueue(args, newAbsolutePath);
        if (rc_pu < SUCCESS) {
          rc_t = MALLOC_FAILURE;
        }
      }
    }
  }

  return rc_t;
}
