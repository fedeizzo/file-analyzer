#ifndef __TUI_H__
#define __TUI_H__

#include "../list/list.h"
#include <pthread.h>
#include "../config/config.h"

/**
 * Holds screen info and content
 *
 * fields:
 *    char **grid: screen grid
 *    int cols: number of columns of the grid
 *    int rows: number of rows of the grid
 *    int cmd: command mode number
 */
typedef struct ScreenStr {
  char **grid;
  int cols;
  int rows;
  int cmd;
} * Screen;

/**
 * Holds mutex for thread
 *
 * fields:
 *    pthread_mutex_t mutex: the mutex
 *    Scren screen: the screen
 */
struct screenMutex {
  pthread_mutex_t mutex;
  Screen screen;
};
typedef struct screenMutex screenMutex_t;

/**
 * termios utility for graphic
 */
void reset_input_mode(void);

/**
 * termios utility for graphic
 */
void set_input_mode(void);

/**
 * Creates a screen
 *
 * args:
 *    int width: width of the screen
 *    int heigth: heigth of the screen
 *
 * returns:
 *    the Screen created in case of success, NULL otherwise
 */
Screen newScreen(int width, int heigth);

/**
 * Destroys the screnn passed as argument
 *
 * args:
 *    Screen screen: the screen for the destroy operation
 */
void destroyScreen(Screen screen);

/**
 * Spawn a process that read the width of the screen
 *
 * args:
 *    int *width: variable where width are saved
 *
 * returns:
 *    0 in case of succes, negative otherwise
 */
int getWidth(int *width);

/**
 * Spawn a process that read the heigth of the screen
 *
 * args:
 *    int *width: variable where heigth are saved
 *
 * returns:
 *    0 in case of succes, negative otherwise
 */
int getHeigth(int *heigth);

/**
 * Clears the screen
 */
void clear();

/**
 * Moves the cursor to a specific position
 *
 * args:
 *    int x: the column of the position
 *    int y: the row of the position
 */
void moveCursor(int x, int y);

/**
 * Writes a string inside the screen from a specific position
 *
 * args:
 *    Screen screen: the screen where write operation is made
 *    char *str: the string for the write operation
 *    int x: the column of the position
 *    int y: the row of the position
 */
void writeScreen(Screen screen, char *str, int x, int y);

/**
 * Inserts border and other chars inside the screen grid
 *
 * args:
 *    Screen screen: the screen where write operation is made
 *
 * returns:
 *    0 in case of success, negative otherwise
 */
int insertBorder(Screen screen);

/**
 * Writes an error string inside the screen
 *
 * args:
 *    char *str: the string for the write operation
 */
void writeScreenError(char *str);

/**
 * Writes a log string inside the screen
 *
 * args:
 *    int heigth: heigth for the positioning
 *    char *str: the string for the write operation
 */
void writeScreenLog(int heigth, char *str);

/**
 * Prints LOG/ERROR from other components of the programm
 *
 * args:
 *    Screen screen: the screen where write operation is made
 *    int fd: file descriptor from which LOG are read
 *
 * returns:
 *    0 in case of success, negative otherwise
 */
int printLog(Screen screen, int fd);

/**
 * Filters the correct chars that must be displayed on the screen based on the mode
 *
 * args:
 *    const ind cmd: the mode
 *    const int counter: the char code (ASCII)
 */
int commandFilter(const int cmd, const int counter);

/**
 * Draws the screen grid
 *
 * args:
 *    Screen screen: the screen that holds the grid
 */
void draw(Screen screen);
int checkCommand(char *cmd);
int checkFile(char *path);

/**
 * Gets keyboard events asynchronously for input operation
 *
 * returns:
 *    the key code pressed
 */
int getkey();

/**
 * Calls insertBorder and draws some messagges on the screen grid
 * 
 * args:
 *    Screen screen: the screen where insert opeartions are made
 *
 * returns:
 *    0 in case of success, negative otherwise
 */
int initScreen(Screen screen);

/**
 * Thread loop that holds draw operations of the screen
 *
 * args:
 *    void *ptr: pointer to mutex struct at the start of this file
 */
void *graphicsLoop(void *ptr);

/**
 * Mantains high refresh rate for user input fields
 *
 * args:
 *    Screen screen: the screen for the refresh operation
 */
void drawInputLine(Screen screen);

/**
 * Draws the counting table
 *
 * args:
 *    Screen screen: the screen for the write operation
 */
void updateTable(Screen screen, unsigned long long *table);

/**
 * Thread loop that holds user's input opoerations
 *
 * args:
 *    void *ptr: pointer to mutex struct at the start of this file
 */
void *inputLoop(void *ptr);

/**
 * Handles the options passed from command line interface
 *
 * args:
 *    List args: list where files and folder passed are saved
 *    cons int argc: the amount of options passed
 *    char **argv: the options passed
 *    int *n: the variable where manager number is saved
 *    in *m: the variable where worker number is saved
 *    int *mode: the variable where running mode is saved
 */
int optionsHandler(List args, const int argc, char **argv, int *n, int *m,
                   int *mode);
#endif
