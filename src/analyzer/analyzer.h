#ifndef __ANALYZER_H__
#define __ANALYZER_H__

#include <pthread.h>
#include "../tree/tree.h"

/**
 * Data structure that rapresent a file that can be a directory or not
 */
typedef struct FileInfo{
    char *name;
    int *fileTable;
    char *path;
    int isDirectory;
} *FileInfo;

typedef struct sharedResourcesAnalyzer {
  Tree fs;
  List managers; //TODO... Change as priprity queue
  List fileToAssign;
  TreeNode currentDirectory;
  char *cwd;
  int *nManager;
  pthread_mutex_t mutex;
} sharedResourcesAnalyzer_t;

#endif
