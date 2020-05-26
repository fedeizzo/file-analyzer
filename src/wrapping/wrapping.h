#ifndef __WRAPPING_H__
#define __WRAPPING_H__

#include "../config/config.h"

/**
 * Wrap function to print error in the stderr
 *
 * args:
 *    const char *msg: error message
 */
void printError(const char *msg);

/**
 * Wrap function to print info in the stderr
 *
 * args:
 *    const char *msg: info message
 */
void printInfo(const char *msg);

/**
 * Wrap fucntion to check correct allocation with malloc
 *
 * args:
 *    void *ptr: pointer to check
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int checkAllocationError(void * ptr);

/**
 * Wrap function to open a file
 *
 * args:
 *    const char *path: path to the file
 *    const int mode: integer represents mode in which file must be opened (es. O_RDONLY)
 *
 * returns:
 *    number of descriptor in case of success, otherwise -1
 */
int openFile(const char *path, const int mode);

/**
 * Wrap function to move cursor inside a file
 *
 * args:
 *    const char fd: file descriptor
 *    const int position: offset
 *
 * returns:
 *    new position of the cursor in case of success, otherwise -1
 */
long long moveCursorFile(const int fd, const unsigned long long position, const int absPosition);

/**
 * Wrap function to read a char from a descriptor
 *
 * args:
 *    const int fd: descriptor
 *    char *dst: destination for the char
 *
 * returns:
 *    1 in case of success, otherwise -1
 */
int readChar(const int fd, char *dst);

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
 *    len: length to read
 * 
 * returns:
 *    number of bytes read in case of success, otherwise -1
 */
int readDescriptor(const int fd, char dst[], const int len);

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
int writeDescriptor(const int fd, const char msg[]);

/**
 * Wrap function for dup2
 *
 * args:
 *    const int writer: the replacement file desciptor
 *    const int overwritten: the replaced file descriptor
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int createDup(const int writer, const int overwritten);

/**
 * Wrap function to create an anonymous unidirectional pipe
 *
 * args:
 *    int fd[]: array of of int where pipe's descriptor are saved
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int createUnidirPipe(int fd[]);

/**
 * Wrap function to create an anonymous bidirectional pipe
 *
 * args:
 *    int fd[]: array of of int where pipe's descriptor are saved
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int createBidirPipe(int pipe1[], int pipe2[]);

/**
 * Wrap function to create an anonymous unidirectional pipe
 *
 * args:
 *    const int fd[]: array of of int where pipe's descriptor are saved
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int parentInitUniPipe(const int fd[]);

/**
 * Wrap function to create an anonymous unidirectional pipe
 *
 * args:
 *    const int fd[]: array of of int where pipe's descriptor are saved
 * 
 * returns:
 *    0 in case of success, otherwise -1
 */
int childInitUniPipe(const int fd[]);
int childWriteUniPipe(const int fd[], const char *msg);
int parentReadUniPipe(const int fd[], char *dst);

/**
 * Wrap function to create an anonymous bidirectional pipe
 *
 * args:
 *    const int fd[]: array of of int where pipe's descriptor are saved
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int parentInitBidPipe(const int fd[]);

/**
 * Wrap function to create an anonymous bidirectional pipe
 *
 * args:
 *    const int fd[]: array of of int where pipe's descriptor are saved
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int childInitBidPipe(const int fd[]);

/**
 * Wrap function to destroy an anonymous bidirectional pipe
 *
 * args:
 *    const int fd[]: array of of int where pipe's descriptor are saved
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int parentDestroyBidPipe(const int fd[]);

/**
 * Wrap function to destroy an anonymous bidirectional pipe
 *
 * args:
 *    const int fd[]: array of of int where pipe's descriptor are saved
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int childDestroyBidPipe(const int fd[]);

/**
 * Wrap function to write from child to parent through bidirectional pipe
 *
 * args:
 *    const int fd[]: descriptors
 *    char msg[]: message for writing operation
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int childWriteBidPipe(const int fd[], const char *msg);

/**
 * Wrap function to write from parent to child through bidirectional pipe
 *
 * args:
 *    const int fd[]: descriptors
 *    char msg[]: message for writing operation
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int parentWriteBidPipe(const int fd[], const char *msg);

/**
 * Wrap function to allow child read msg from parent
 *
 * args:
 *    const int fd: descriptor
 *    char dst[]: destination of the reading operation
 * 
 * returns:
 *    number of bytes read in case of success, otherwise -1
 */
int childReadBidPipe(const int fd[], char *dst);

/**
 * Wrap function to allow parent read msg from child
 *
 * args:
 *    const int fd: descriptor
 *    char dst[]: destination of the reading operation
 * 
 * returns:
 *    number of bytes read in case of success, otherwise -1
 */
int parentReadBidPipe(const int fd[], char *dst);

#endif 
