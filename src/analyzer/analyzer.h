#ifndef __ANALYZER_H__
#define __ANALYZER_H__

#include <pthread.h>
#include "../tree/tree.h"
#include "../priorityQueue/priorityQueue.h"

/**
 * Data structure that rapresent a file that can be a directory or not
 */
typedef struct FileInfo{
    char *name;
    unsigned long long *fileTable;
    char *path;
    int isDirectory;
} *FileInfo;

typedef struct sharedResourcesAnalyzer {
  Tree fs;
  PriorityQueue managers;
  List fileToAssign;
  List candidateNode;
  TreeNode currentDirectory;
  char *cwd;
  char *path;
  int *nManager;
  int *nWorker;
  pthread_mutex_t mutex;
} sharedResourcesAnalyzer_t;

typedef struct TreeNodeCandidate{
  TreeNode startingNode;
  int type; 
  char *path;
  int toSkip;
} *TreeNodeCandidate;

#endif
