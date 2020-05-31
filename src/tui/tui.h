#ifndef __TUI_H__
#define __TUI_H__

#include <pthread.h>

#include "../config/config.h"
#include "../list/list.h"


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
  int treeStartCol;
  int treeEndCol;
  int treeStartRow;
  int treeEndRow;
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
 *    int width : width of the screen
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
 * Checks if two string is equal (used in List)
 *
 * args:
 *    void *data : first string to compare
 *    void *data2: second string to compare
 */
int isStringEqual(void *data, void *data2);

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
 *    char *str    : the string for the write operation
 *    int x        : the column of the position
 *    int y        : the row of the position
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
 * Filters the correct chars that must be displayed on the screen based on the mode
 *
 * args:
 *    const ind cmd    : the mode
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
 * Computes statistics calculus reading table
 *
 * args:
 *    Screen screen            : the screen where calculus are written
 *    unsigned long long *table: the table from which informations are read
 */
void computeStatistics(Screen screen, unsigned long long *table);

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
 * Clears the center of the screen
 *
 * args:
 *    Screen screen: the screen for the refresh operation
 */
void clearCenter(Screen screen);

/**
 * Draws the counting table
 *
 * args:
 *    Screen screen            : the screen for the write operation
 *    unsigned long long *table: the table from which informations are read
 */
void updateTable(Screen screen, unsigned long long *table);

/**
 * Draws help message in the center of the screen
 *
 * args:
 *    Screen screen: the screen for the refresh operation
 */
void drawHelpMsg(Screen screen);

/**
 * Draws the center of the screen
 *
 * args:
 *    Screen screen            : the screen for the refresh operation
 *    unsigned long long *table: the table from which informations are read
 */
void drawCenter(Screen screen, unsigned long long *table);

/**
 * Trims a string removing whitespace after last non space char
 *
 * args:
 *    char *string: the string for trim operation
 */
void trim(char *string);

/**
 * Checks if the screen must be resized
 *
 * args:
 *    int *oldHeigth: the old height value
 *    int *oldWidth : the old width value
 *    int *height   : the new height value
 *    int *width    : the new width value
 *
 * returns:
 *    0 in case of success, otherwise negative
 */
int resize(int *oldHeigth, int *oldWidth, int *heigth, int *width);

/**
 * Changes command mode based on input as argument
 *
 * args:
 *    Screen screen: the screen where some infos are printed
 *    int *cmd     : the input from user
 *    int cmdMode  : a number that represents the mode
 *    int *row     : the row where cursor is moved after the update
 *    int *column  : the column where cursor is moved after the update
 */
void changeCommandMode(Screen screen, int *cmd, int cmdMode, int *row,
                       int *column);

/**
 * Changes the component amount passed as argument
 *
 * args:
 *    Screen screen       : the screen where some infos are printed
 *    char *cmd           : the input written by the user
 *    int *componentAmount: the component that must be changed
 *    int *row            : the row where the cursor is moved after the update
 *    int *column         : the column where the cursor is moved after update
 */
void changeComponentAmount(Screen screen, char *cmd, int *componentAmount,
                           int *row, int *column);

/**
 * Thread loop that holds user's input operations
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
int optionsHandler(List args, const char *cwd, const int argc, char **argv, int *n, int *m,
                   int *mode);
#endif
