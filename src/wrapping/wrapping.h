#ifndef __WRAPPING_H__
#define __WRAPPING_H__

/**
 * Wrap function to print error in the sterr
 *
 * args:
 *    const char *msg: error message
 */
void printError(const char *msg);

/**
 * Wrap function to open a file
 *
 * args:
 *    const char *path: path to the file
 *    const int mode: integer represents mode in which file must be opened (es. O_RDONLY)
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int openFile(const char *path, const int mode);

/**
 * Wrap function to close a descriptor
 *
 * args:
 *    const int fd: descriptor
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int closeDescriptor(const int fd);

/**
 * Wrap function to read from a descriptor
 *
 * args:
 *    const int fd: descriptor
 *    char dst[]: destination of the reading operation
 * 
 * returns:
 *    number of bytes read in case of success, otherwise -1
 */
int readDescriptor(const int fd, char dst[]);

/**
 * Wrap function to write to a descriptor
 *
 * args:
 *    const int fd: descriptor
 *    char msg[]: message for writing operation
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int writeDescriptor(const int fd, char msg[]);

/**
 * Wrap function to create an anonymous pipe
 *
 * args:
 *    int fd[]: array of of int where pipe's descriptor are saved
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int createPipe(int fd[]);

/**
 * Wrap function to create an anonymous pipe
 *
 * args:
 *    const int fd[]: array of of int where pipe's descriptor are saved
 *    const char *msg: message for writing operation
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int parentWrite(const int fd[], const char *msg);

/**
 * Wrap function to create an anonymous pipe
 *
 * args:
 *    const int fd[]: array of of int where pipe's descriptor are saved
 *    char *dst: message for reading operation
 *
 * returns:
 *    number of bytes read in case of success, otherwise -1
 */
int childRead(const int fd[], char *dst);

#endif 
