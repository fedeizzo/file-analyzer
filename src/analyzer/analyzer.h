#ifndef __ANALYZER_H__
#define __ANALYZER_H__

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
  
  pthread_mutex_t mutex;
} sharedResourcesAnalyzer_t;

#endif
