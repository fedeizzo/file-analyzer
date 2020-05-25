#include "./analyzer.h"
#include "../config/config.h"
#include "../manager/manager.h"
#include "../priorityQueue/priorityQueue.h"
#include "../table/table.h"
#include "../tree/tree.h"
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/**
 * Initializes the root of the Tree
 *
 * args:
 *    int *msg: message used to check error in the root initialization
 *
 * returns
 *    A FileInfo with the initialized Root or NULL
 */
FileInfo initFileInfoRoot(int *msg);

/**
 * Function that creates a new FileInfo
 *
 * args:
 *    char *name: the name of the file (it won't be copied so the original data mustn't be freed)
 *    int isDirectory: parameter to specify if the informations are about a file (ISFILE) or a directory (DIRECTORY)
 *    char *path: the complete path of the file, it can be relative from the program folder or absolute (it will be copied with a strcpy so the original data must be freed afterwords)
 *    int *msg: message used to check error in the FileInfo creation
 *
 * returns
 *    A specific FileInfo with the name, isDirectory and path passed as parameters
 */
FileInfo newFileInfo(char *name, int isDirectory, char *path, int *msg);

/**
 * Function that allocate a newPath for a FileInfo, if the passed path is NULL then FileInfo's
 * path paramether will be set to NULL too
 * 
 * args:
 *    FileInfo fileinfo: a FileInfo with no previous path element allocated in the structure
 *    char *string: the path (which will be copied) for the first parameter FileInfo
 *
 * returns
 *    SUCCESS otherwhise a negative number to inform malloc or strcpy failure
 */
int allocatePath(FileInfo fileinfo, char *string);

/**
 * Destructor for a FileInfo instance
 * 
 * args:
 *    void *toDestroy: a FileInfo instance which must be destroyed
 *
 */
void destroyFileInfo(void *toDestroy);

/**
 * Function that checks if the strcpy function went rigth or not
 * 
 * args:
 *    char *dst: a reference to the memory where will be copied the src string
 *    char *src: the string which will be copied
 *
 * returns
 *    SUCCESS if strcpy didn't fail otherwise FAILURE
 */
int checkStrcpy(char *dst, char *src);

/**
 * Function that creates the Tree and initializes it for the Analyzer 
 * 
 * args:
 *    FileInfo data: the info which will be passed to the Tree to create the root
 *    int *msg: message used to check for errors in the newTree function
 *    void destroyer(void *): a void function which accept a void * parameter and will be used to destroy
 *      the data of the TreeNode 
 *
 * returns
 *    An instance of the initialized Tree or NULL 
 */
Tree initializeAnalyzerTree(FileInfo data, int *msg, void destroyer(void *));

/**
 * Function that insert a TreeNode inside the tree searching from a starting point for the least common ancestor
 * which has been already inserted in the tree and then it adds the remaining files to the tree.
 * 
 * args:
 *    char *path: a compacted path to a file which is returned from a bash call to the find function
 *    char *completePath: a complete path to a file which is returned from a bash call to the find function
 *    TreeNode startingPoint: the TreeNode (aka the directory) from where it will be started a search for
 *      the least common ancestor
 *    int isDirectory: parameter to specify if the last element in the complete path is a directory or a file
 *    int *msg: message used to check error in the performInsert function
 *   
 *
 * returns
 *    A TreeNode that points to a TreeNode inside the Tree in order to update the characters table of the file
 *    NULL otherwise 
 */
TreeNode performInsert(char *path, char *completePath, TreeNode startingPoint,
                       int isDirectory, int *msg);

/**
 * Function that obtains the least common ancestor from the Tree based on the compacted path passed in input
 * 
 * args:
 *    TreeNode toExamine: a TreeNode of the Tree which must be examined and will be used to get their children if
 *      a match happened
 *    FileInfo dataToExamine: the infos of the TreeNode element that must be examined
 *    Node actualNode: a List element which contains the TreeNode to Examined and will be used to get the next child
 *      of the starting point in case of no match with the current TreeNode
 *    char *path: the compacted path obtained form the compactPath function
 *    int *found: pointer to an integer which is used to check if the TreeNode which is being examined
 *      corresponds to the LCA
 *    int *match: pointer to an integer which is used to check if the words in the name of the TreeNode examined
 *      matches that of the path
 *    int *counter: pointer to an integer which is used to keep track of the current word in the path
 *    int *resetCounter: a pointer to an integer which is used to reset the counter parameter in case of not match
 *    int *tmpCounter: a pointer to an integer which is used to keep track of the current word in the TreeNode
 *
 * returns
 *    the Node of the List which contains the TreeNode that was examined in case of match or the
 *    next element in the list (aka the brother of the node the function has examined) 
 */
Node getNodeWhereToInsert(TreeNode toExamine, FileInfo dataToExamine,
                          Node actualNode, char *path, int *found, int *match,
                          int *counter, int *resetCounter, int *tmpCounter);

/**
 * Function that checks if the name in the FileInfo of a specific TreeNode matches a specific part of the
 * compacted path passed as input
 * 
 * args:
 *    FileInfo dataToExamine: the infos of the TreeNode element that must be examined
 *    char *path: the compacted path obtained form the compactPath function
 *    int *counter: pointer to an integer which is used to keep track of the current word in the path
 *    int *resetCounter: a pointer to an integer which is used to reset the counter parameter in case of not match
 *    int *tmpCounter: a pointer to an integer which is used to keep track of the current word in the TreeNode
 *
 * returns
 *    0 in case the two names match or -1 in case they don't 
 */
int checkIfMatch(FileInfo dataToExamine, char *path, int *counter,
                 int *resetCounter, int *tmpCounter);
                 
/**
 * Function that check the value of the checkIfMatch function (also performs other operations to verify if
 * the element is a false positive) and sets (or resets) counters, nodes and the found variables for the next match
 * 
 * args:
 *    TreeNode toExamine: a TreeNode of the Tree which must be examined
 *    FileInfo dataToExamine: the infos of the TreeNode element that must be examined
 *    char *path: the compacted path obtained form the compactPath function
 *    int *found: pointer to an integer which is used to check if the TreeNode which is being examined
 *      corresponds to the LCA
 *    int *match: pointer to an integer which is used to check if the words in the name of the TreeNode examined
 *      matches that of the path
 *    int *counter: pointer to an integer which is used to keep track of the current word in the path
 *    int *resetCounter: a pointer to an integer which is used to reset the counter parameter in case of not match
 *    int *tmpCounter: a pointer to an integer which is used to keep track of the current word in the TreeNode
 *
 * returns
 *    the Node of the first element in the List of the TreeNode the function was checking in case of found == 1 and in
 *    the case it has at least one element otherwise NULL which must be handled in the calling function 
 */
Node handleMatch(TreeNode toExamine, FileInfo dataToExamine, char *path,
                 int *found, int *match, int *counter, int *resetCounter,
                 int *tmpCounter);
/**
 * Function that creates one or more TreeNodes in a way that the first is the parent of the second, the second is the 
 * parent of the third, and so on... based on the remaining path which hasn't matched in the perform insert function
 * 
 * args:
 *    TreeNode whereToInsert: the element of the Tree where it will added the next child 
 *    int *counter: pointer to an integer which is used to keep track of the current word in the path
 *    char *path: the compacted path obtained form the compactPath function
 *    char *pathName: a pointer to a memory location for a string of size char*PATH_MAX which will cointains the name
 *      of the file or the folder (without the complete relative or absolute path)
 *    int isDirectory: parameter to specify if the last element in the complete path is a directory or a file
 *    char *completePath: a string (which will be copied and so the original must be freed afterwords) which contains
 *      the full path (relative from the analyzer starting directory or absolute) to the file 
 *    int *msg: message used to check error in the createNewTwig function
 *
 * returns
 *    A TreeNode to the file which must be examined or NULl 
 */
TreeNode createNewTwig(TreeNode whereToInsert, int *counter, char *path,
                       char *pathName, int isDirectory, char *completePath,
                       int *msg);

/**
 * Function that creates one or more TreeNodes in a way that the first is the parent of the second, the second is the 
 * parent of the third, and so on... based on the remaining path which hasn't matched in the perform insert function
 * 
 * args:
 *    FileInfo dataToInsert: a FileInfo instance where will be stored the data of the file it'll be inserted in
 *      the toInsert TreeNode
 *    TreeNode toInsert: a TreeNode instance which will be inserted in the Tree and linked to the whereToInsert element 
 *    TreeNode whereToInsert: the element of the Tree where it will added the next child 
 *    int *tmpCounter: pointer to an integer which is used to insert the next word in the pathName string
 *    char *pathName: a pointer to a memory location for a string of size char*PATH_MAX which will cointains the name
 *      of the file or the folder (without the complete relative or absolute path)
 *    int isDirectory: parameter to specify if the last element in the complete path is a directory or a file
 *    char *completePath: a string (which will be copied and so the original must be freed afterwords) which contains
 *      the full path (relative from the analyzer starting directory or absolute) to the file 
 *    int *msg: message used to check error in the createNewTreeElement function
 *
 * returns
 *    A TreeNode to the file inserted in the Tree which will be used as dataToInsert for the next iteration 
 */
TreeNode createNewTreeElement(FileInfo dataToInsert, TreeNode toInsert,
      TreeNode whereToInsert, int *tmpCounter,
      char *pathName, int isDirectory,
      char *completePath, int *msg);

/**
 * loop for reading the directives from the user or from the reporter
 *
 * args:
 *    void *ptr: args for thread
 */
void *readDirectivesLoop(void *ptr);

// TODO... check the returning value of this function cause it's always SUCCESS
/**
 * Function that waits (in a non blocking way) for user's input (or reporter comunication) for 3 parameters and then
 * casts two of them to integer
 * 
 * args:
 *    char *path: a pointer to a memory location to store the path of a file or a folder passed in input
 *    int *nManager: a pointer to the the number of manager which will be spawned to handle the file passed in input
 *    int *nWorker: a pointer to the the number of worker for each manager which will be spawned to handle
 *      the file computation of the passed path 
 *
 * returns
 *    SUCCESS in any case
 */
int readDirectives(char *path, char *nManager, char *nWorker);

/**
 * Function that changes the amount of total managers by adding or substracting them to the current number
 * 
 * 
 * args:
 *    PriorityQueue managers: priority queue containing the information of each manager
 *    const int currentManagers: the current number of managers which are waiting or working on one or more files
 *    const int newManagers: the new number of managers which will become the current number after saving the work
 *      of the managers which will be removed or after spawning the new ones
 *    List fileToAssign: a list of files which will be afterword assigned to the managers
 *
 * returns
 *    SUCCESS or a negative number which must be handled and can means a problem while allocating the memory (for
 *    different reasons) or while extracting the infos from the managers which will be deleated
 */
int changeManagersAmount(PriorityQueue managers, const int currentManagers,
                         const int newManagers, List fileToAssign);

/**
 * Function that adds a number (specified by amount) of managers to the current ones and stores their informations
 * in the respective priority Queue
 * 
 * 
 * args:
 *    PriorityQueue managers: priority queue containing the information of each manager
 *    int amount: the number of managers which will be added to the current ones
 *
 * returns
 *    SUCCESS or a negative number which must be handled and can means a problem while allocating the memory (for
 *    different reasons), errors that can be associated to the list or relative to the comunication pipe
 */
int addManagers(PriorityQueue managers, int amount);

/**
 * Inits Manager's pipes
 *
 * args:
 *    const int readPipe[]: read pipe
 *    const int writePipe[]: write pipe
 *
 * returns
 *    0 in case of success, negative number otherwise
 */
int managerInitPipe(const int toParent[], const int toChild[]);

/**
 * Function that creates a new manager intializing the pipes, the list of files which will be later sends for a
 * future execution and his pid
 * 
 * 
 * returns
 *    SUCCESS otherwise -1 if there's a problem while using malloc
 */
Manager newManager();

/**
 * Function which removes a specific amount of managers and stores their files into a list for be re-assigned
 * afterwords, it then collects the files from the remaining managers in order to re-distribute the amount
 * of works between them and informs them to stop doing their work
 * 
 * 
 * args:
 *    PriorityQueue managers: priority queue containing the information of each manager
 *    int amount: the number of managers which will be removed from the current ones
 *    List fileToAssign: a list of files which will be used afterword assigned to the managers
 *
 * returns
 *    SUCCESS or a negative number which must be handled and can means a problem while removing the managers
 *    from the list or removing/storing their files
 */
int removeManagers(PriorityQueue managers, int amount, List fileToAssign);

/**
 * Function which removes all the files from the list of a specific manager in order to store them
 * into a List and re-distribute the files in a second moment
 * 
 * 
 * args:
 *    Manager m: the Manager from which the files will be removed
 *    List fileToAssign: a list of files which will be used afterword assigned to the managers
 *
 * returns
 *    SUCCESS or a negative number which must be handled and can means a problem while swapping or appending the
 *    list or an error while allocating the list at the start
 */
int endManager(Manager m, List fileToAssign);

/**
 * Function which closes the comunication pipe between the manager and the analyzer and does frees
 * some informations about the manager such as the ppipe or the main pointer to the list (which must be deallocated
 * in a separate action in order to not lose information about the files)
 * 
 * 
 * args:
 *    void *data: a void pointer to a Manager instance
 */
void destroyManager(void *data);

/**
 * Function which informs the manager to stop processing their files by sending two "stop" message
 * 
 * 
 * args:
 *    Manager m: the Manager which will be informed to stop processing the list of files
 *
 * returns
 *    SUCCESS or a negative number which must be handled and can means a problem allocating the memory for the
 *    message, while sending the message using the pipe or casting it using sscanf
 */
int informManager(Manager manager);

/**
 * Function which precomputes the path recived from the users (or the reporter) identifying if it's ABSOLUTE or
 * RELATIVE while compacting it in order to find the starting directory
 * 
 * 
 * args:
 *    char *cwd: a string containing the path to the current working directory
 *    char *path: a string containing the path which will be compacted in order to avoid useless directory loops
 *    int *msg: message used to check error in the precomputeAnalyzerInput function
 *
 * returns
 *    The type of the path which was passed in input to the function (RELATIVE or ABSOLUTE)
 */
int precomputeAnalyzerInput(char *cwd, char *path, int *msg);

/**
 * Function which compacts the path passed in input from the user (or the reporter) in order to avoid loops of some
 * kind passed in input by calling an istance of BashCall
 * 
 * 
 * args:
 *    char *path: a string containing the path which will be compacted in order to avoid useless directory loops
 *    char *cwd: a string containing the path to the current working directory
 *    int isAbsolute: an integer which specifies if the path is RELATIVE or ABSOLUTE
 *
 * returns
 *    SUCCESS otherwise an error while reading from the realpath bash call
 */
int compactPath(char *path, char *cwd, int isAbsolute);

/**
 * Function that executes a fork and the child execute the command passed as parameter while the father reads from a non blocking pipe
 * the informations returned
 * 
 * 
 * args:
 *    char *dst: a pointer to a memory location which will store the infromations returned from the child
 *    char *argv[]: a vector of string which will be used as argument for the execpv call 
 *
 * returns
 *    SUCCESS otherwise a negative number which must be handled and can means an error while forking or reading/writing on the pipe
 */
int callBashProcess(char *dst, char *argv[]);

/**
 * Function that initializes the pipe for the child and then executes a execvp with the argument specified in *argv[] 
 * 
 * 
 * args:
 *    int *fd: the file descriptor created using the pipe function in the calling function
 *    char *argv[]: a vector of string which will be used as argument for the execpv call 
 *
 * returns
 *    SUCCESS otherwise a negative number which indicates an error while executing execvp
 */
int writeBashProcess(int *fd, char *argv[]);

// TODO... remember to fix this return code
/**
 * Function that reads the informations writed by the child on a non blocking pipe
 * 
 * 
 * args:
 *    int *fd: the file descriptor created using the pipe function in the calling function
 *    char *dst: a pointer to a memory location which will store the infromations returned from the child
 *    int childPid: the pid of the child created in the calling function in order to verify if it's not dead and not be stucked in the
 *      non blocking pipe
 *
 * returns
 *    SUCCESS and only success at the moment
 */
int readBashLines(int *fd, char *dst, int childPid);

/**
 * Function that sets up a bash call in order to retrive the type of the path passed in input (if it's a file, a folder or it
 * doesn't exist in that particular scope or at all) 
 * 
 * 
 * args:
 *    char *name: the name (or the path) of a file or a folder which is passed in input 
 *
 * returns
 *    SUCCESS otherwise a negative number which must be handled and indicates an error while processing the informations retrived
 *    from the bash call
 */
int checkFileType(char *name);

/**
 * Function that adds a forward slash in case it's not already present in the valid path passed as input
 * 
 * 
 * args:
 *    char *path: the valid path of a file or a folder which is passed in input 
 */
void addForwardSlash(char *path);

/**
 * Function that precomputs the starting point, based on the path passed in input, starting from the cwd (or from the root of the file
 * sistem stored in the tree if it starts with a forward slash)
 * 
 * args:
 *    Tree fs: the Tree which rapresents the file system discovered so far based on the user's request
 *    TreeNode currentDirectory: the TreeNode (aka the directory) from where the main program has been executed
 *    char *path: the compacted path to an existing file (or directory)
 *    int *toSkip: a pointer to an integer which will be initialized to the number of characters to skip because are not part of the
 *      file or folder name and only useful for the navigation inside the tree
 *   
 *
 * returns
 *    A TreeNode that points to an element of Tree which will be used as starting point for search
 */
TreeNode precomputeStartingDirectory(Tree fs, TreeNode currentDirectory,
                                     char *path, int *toSkip);

/**
 * Function that handle the multiple instance of .. inside the path in order to get at the highest level
 * 
 * args:
 *    Tree fs: the Tree which rapresents the file system discovered so far based on the user's request
 *    TreeNode currentDirectory: the TreeNode (aka the directory) from where the main program has been executed
 *    char *path: the compacted path to an existing file (or directory)
 *    int *toSkip: a pointer to an integer which will be initialized to the number of characters to skip because are not part of the
 *      file or folder name and only useful for the navigation inside the tree
 *   
 *
 * returns
 *    A TreeNode that points to an element of Tree which will be used as starting point for search
 */                                  
TreeNode goUp(Tree fs, TreeNode currentDirectory, char *path, int *toSkip);

/**
 * Function that handle the multiple instance of .. inside the path in order to get at the highest level
 * 
 * args:
 *    TreeNode startingNode: the starting directory where the search inside the tree will start
 *    int typeOfFile: the type of the file which will be analyzed (ISFILE or DIRECTORY)
 *    char *path: the valid path of a file or a folder which is passed in input 
 *    int characterToSkip:  number of characters to skip because are not part of the file or folder name and
 *      only useful for the navigation inside the tree
 *   
 *
 * returns
 *    A TreeNodeCandidate which will be put later on inside a list in order to be analyzed
 */
TreeNodeCandidate newTreeNodeCandidate(TreeNode startingNode, int typeOfFile,
                                       char *path, int characterToSkip);

/**
 * Function that destroys a TreeNodeCandidate (N.B. not all data inside the TreeNodeCandidate will be freed
 * because they'll be used in the next operations)
 * 
 * args:
 *    void *data: the TreeNodeCandidate that needs to be destroyed
 */
void destroyTreeNodeCandidate(void *data);

int spawnFindProcess(char *compactedPath, int *fd, int *childPid);

int scheduleFile(TreeNode toSchedule, Manager manager);

int insertAndSchedule(TreeNode startingNode, List filesToAssign,
                      char *relativePath, char *completePath, List requestedFiles);

int slicePath(TreeNode currentDirectory, PriorityQueue managers,
              List filesToAssign, int *fd, char *relativePath,
              char *completePath, int toSkip, const int *nManager,
              const int *nWorker, int childPid);

int parseLineDescriptor(TreeNode currentDirectory, char *relativePath, int *fd,
                        int toSkip, PriorityQueue managers, const int *nManager,
                        const int *nWorker, List filesToAssign, int childPath);

int saveFindResult(TreeNode currentDirectory, char *compactedPath,
                   char *relativePath, int toSkip, int isDirectory,
                   PriorityQueue managers, const int *nManager,
                   const int *nWorker, List filesToAssign);

int saveFiles(Tree fs, TreeNode currentDirectory, char *compactedPath,
              int toSkip, int isDirectory, PriorityQueue managers,
              const int *nManager, const int *nWorker, List filesToAssign);

int checkAnalyzeAndAssign(Tree fs, TreeNode currentDirectory, char *path,
                          int *toSkip, PriorityQueue managers,
                          const int *nManager, const int *nWorker,
                          List filesToAssign);

int doWhatAnalyzerDoes(char *cwd, Tree fs, TreeNode currentDirectory,
                       PriorityQueue managers, int *nManager, int *nWorker,
                       List fileToAssign, char *path);

int parentInitExecPipe(const int toParent[], const int toChild[]);

int sendFile(Manager manager, TreeNode file, List filesToAssign,
             int currentWorker);

int rescheduleFiles(List toDestroy, List toReschedule);

int manageFileToSend(PriorityQueue managers, int currentWorker,
                     List filesToAssign);

int getCwd(char *dst);

void *fileManageLoop(void *ptr);

void *sendFileLoop(void *ptr);

void skipPath(char *path, char *relativePath, int toSkip);

void toStringTable(long long unsigned *table);

int respawnManager(PriorityQueue managers, Manager dead, List fileToAssign);

FileInfo initFileInfoRoot(int *msg) {
  FileInfo rootData;
  char *rootName = (char *)malloc(sizeof(char) * 2);
  int rc_ca = checkAllocationError(rootName);
  int rc_nc = SUCCESS;
  if (rc_ca < 0) {
    printError("I can't allocate memory");
  } else {
    if (strcpy(rootName, "/") != NULL)
      rootData = newFileInfo((void *)rootName, DIRECTORY, NULL, &rc_nc);
  }
  // Error checking
  if (rc_ca == MALLOC_FAILURE || rc_nc == MALLOC_FAILURE) {
    *msg = MALLOC_FAILURE;
  }
  return rootData;
}

FileInfo newFileInfo(char *name, int isDirectory, char *path, int *msg) {
  FileInfo toRtn = (FileInfo)malloc(sizeof(struct FileInfo));
  *msg = checkAllocationError(toRtn);
  if (*msg == SUCCESS) {
    toRtn->name = name;
    toRtn->isDirectory = isDirectory;
    toRtn->fileTable =
        (unsigned long long *)calloc(NCHAR_TABLE, sizeof(unsigned long long));
    toRtn->isRequested = -1;
    *msg = checkAllocationError(toRtn->fileTable);
    if (*msg == SUCCESS) {
      *msg = allocatePath(toRtn, path);
      if (*msg != SUCCESS) {
        free(toRtn->fileTable);
        free(toRtn);
      }
    } else {
      free(toRtn);
    }
  }
  return toRtn;
}

int allocatePath(FileInfo fileinfo, char *string) {
  int rc_a = SUCCESS;
  if (string != NULL) {
    fileinfo->path = (char *)malloc(sizeof(char) * PATH_MAX);
    rc_a = checkAllocationError(fileinfo->name);
    if (rc_a == SUCCESS) {
      rc_a = checkStrcpy(fileinfo->path, string);
    }
  } else {
    fileinfo->path = NULL;
  }
  return rc_a;
}

void destroyFileInfo(void *toDestroy) {
  FileInfo data = (FileInfo)toDestroy;
  free(data->name);
  free(data->path);
  free(data->fileTable);
  free(toDestroy);
}

int checkStrcpy(char *dst, char *src) {
  int rc_cp = SUCCESS;
  if (strcpy(dst, src) == NULL) {
    rc_cp = FAILURE;
  }
  return rc_cp;
}

Tree initializeAnalyzerTree(FileInfo data, int *msg, void destroyer(void *)) {
  Tree tree = NULL;
  tree = newTree((void *)data, msg, destroyer, NULL);
  if (*msg != SUCCESS) {
    char *msgErr = (char *)malloc(PATH_MAX);
    int rc_ca = checkAllocationError(msgErr);
    if (rc_ca < 0) {
      printError("Cold not allocate memory to print the error message");
    } else {
      sprintf(msgErr, "Can't create tree inside Analyzer: %d", getpid());
      printInfo(msgErr);
      free(msgErr);
    }
  }
  return tree;
}

TreeNode performInsert(char *path, char *completePath, TreeNode startingPoint,
                       int isDirectory, int *msg) {
  Node actualNode = NULL;
  Node tmp = NULL;
  TreeNode toRtn = NULL;
  TreeNode toExamine = NULL;
  TreeNode whereToInsert = NULL;
  FileInfo dataToExamine = NULL;
  int rc_ca = SUCCESS;
  int found = 0;
  int match = 0;
  int counter = 0;
  int tmpCounter = 0;
  int resetCounter = 0;
  if (startingPoint != NULL && path != NULL) {
    if (isEmptyList(startingPoint->children) == NOT_EMPTY) {
      actualNode = startingPoint->children->head;
      while (found == 0 && actualNode != NULL) {
        match = 0;
        toExamine = (TreeNode)actualNode->data;
        if (toExamine != NULL) {
          dataToExamine = (FileInfo)toExamine->data;
          actualNode = getNodeWhereToInsert(
              toExamine, dataToExamine, actualNode, path, &found, &match,
              &counter, &resetCounter, &tmpCounter);
        }
        tmpCounter = 0;
      }
    }
    if (found == 0) { // The new Element wasn't a children of mine then i need
                      // to add it to my children
      if (toExamine ==
          NULL) { // I need to attach the children to the Starting Point
        whereToInsert = startingPoint;
      } else { // The new node is my brother so I need to attach the children to
               // my father
        whereToInsert = toExamine->parent;
      }
    } else if (found == 1) { // The element it's my first child
      whereToInsert = toExamine;
    }
    if (found != -1) {
      char *pathName = (char *)malloc(sizeof(char) * PATH_MAX);
      rc_ca = checkAllocationError(pathName);
      if (rc_ca == SUCCESS) {
        toRtn = createNewTwig(whereToInsert, &counter, path, pathName,
                              isDirectory, completePath, &rc_ca);
      }
      *msg = rc_ca;
    } else {
      toRtn = toExamine;
      *msg = ALREADY_INSERTED;
    }
  } else {
    *msg = NULL_POINTER;
  }
  return toRtn;
}

Node getNodeWhereToInsert(TreeNode toExamine, FileInfo dataToExamine,
                          Node actualNode, char *path, int *found, int *match,
                          int *counter, int *resetCounter, int *tmpCounter) {
  if (dataToExamine != NULL) {
    if (dataToExamine->name != NULL) {
      // *tmpCounter);
      *match =
          checkIfMatch(dataToExamine, path, counter, resetCounter, tmpCounter);
      Node tmp = handleMatch(toExamine, dataToExamine, path, found, match,
                             counter, resetCounter, tmpCounter);
      if (tmp != NULL) {
        actualNode = tmp;
      } else { // Case no match
        actualNode = actualNode->next;
      }
    }
  }
  return actualNode;
}

int checkIfMatch(FileInfo dataToExamine, char *path, int *counter,
                 int *resetCounter, int *tmpCounter) {
  int match = 0;
  while (path[*counter] != '/' && path[*counter] != '\0' && match == 0) {
    if (path[*counter] == dataToExamine->name[*tmpCounter]) {
      *counter += 1;
      *tmpCounter += 1;
    } else {
      *counter = *resetCounter;
      match = -1;
    }
  }
  return match;
}

Node handleMatch(TreeNode toExamine, FileInfo dataToExamine, char *path,
                 int *found, int *match, int *counter, int *resetCounter,
                 int *tmpCounter) {
  Node actualNode = NULL;
  if (*match == 0) {
    if (dataToExamine->name[*tmpCounter] ==
        '\0') { // Two names matches perfectly
      if (path[*counter] == '\0') { // It's already a folder or a file in the
                                    // actual tree no need to insert again
        *found = -1;
      } else {
        if (isEmptyList(toExamine->children) ==
            NOT_EMPTY) { // If I have some inner files then we need to check if
                         // we can continue to have a match
          actualNode = toExamine->children->head;
        } else { // I've arrived at the deepest known part in the tree which has
                 // already been inserted in the tree
          *found = 1;
        }
        *resetCounter = *counter + 1;
        *counter = *resetCounter;
      }
    } else { // The children name contains the name in the path but they're
             // different
      *counter = *resetCounter;
      *match = -1;
    }
  }
  return actualNode;
}

TreeNode createNewTwig(TreeNode whereToInsert, int *counter, char *path,
                       char *pathName, int isDirectory, char *completePath,
                       int *msg) {
  TreeNode toRtn = NULL;
  TreeNode toInsert = NULL;
  FileInfo dataToInsert = NULL;
  int rc_en = SUCCESS;
  int tmpCounter = 0;
  while (path[*counter] != '\0' && *msg == SUCCESS) {
    if (path[*counter] == '/') {
      if (*counter != 0) { // Check if it's the root
        whereToInsert =
            createNewTreeElement(dataToInsert, toInsert, whereToInsert,
                                 &tmpCounter, pathName, DIRECTORY, NULL, msg);
        tmpCounter = 0;
        if (*msg == SUCCESS) {
          pathName = (char *)malloc(sizeof(char) * PATH_MAX);
          *msg = checkAllocationError(pathName);
        }
      }
    } else {
      pathName[tmpCounter] = path[*counter];
      tmpCounter += 1;
    }
    *counter += 1;
  }
  if (*msg == SUCCESS) {
    toRtn =
        createNewTreeElement(dataToInsert, toInsert, whereToInsert, &tmpCounter,
                             pathName, isDirectory, completePath, msg);
  }
  return toRtn;
}

TreeNode createNewTreeElement(FileInfo dataToInsert, TreeNode toInsert,
                              TreeNode whereToInsert, int *tmpCounter,
                              char *pathName, int isDirectory,
                              char *completePath, int *msg) {
  int rc_nc = SUCCESS;
  int rc_tc = SUCCESS;
  pathName[*tmpCounter] = '\0';
  dataToInsert = newFileInfo(pathName, isDirectory, completePath, &rc_nc);
  toInsert = newTreeNode(whereToInsert, (void *)dataToInsert, &rc_tc);
  if (rc_nc == SUCCESS && rc_tc == SUCCESS) {
    linkChild(whereToInsert, toInsert);
    printf("inserisco %s tra i figli di %s\n", ((FileInfo)toInsert->data)->name, ((FileInfo)whereToInsert->data)->name);
  } else {
    free(dataToInsert);
    free(toInsert);
    *msg = MALLOC_FAILURE;
  }
  return toInsert;
}

void *readDirectivesLoop(void *ptr) {
  sharedResourcesAnalyzer_t *sharedResources = (sharedResourcesAnalyzer_t *)ptr;
  TreeNode startingNode;
  TreeNodeCandidate toAnalyze;
  int rc_t = SUCCESS;
  int rc_rd = SUCCESS;
  int rc_cma = SUCCESS;
  int rc_ct = SUCCESS;
  int rc_an = SUCCESS;
  int rc_al = SUCCESS;
  int rc_al2 = SUCCESS;
  int rc_al3 = SUCCESS;
  int rc_al4 = SUCCESS;
  int rc_al5 = SUCCESS;

  char *newPath = (char *)malloc(sizeof(char) * PATH_MAX);
  rc_al = checkAllocationError(newPath);
  char *nWorker = (char *)malloc(sizeof(char) * PATH_MAX);
  rc_al2 = checkAllocationError(nWorker);
  char *nManager = (char *)malloc(sizeof(char) * PATH_MAX);
  rc_al3 = checkAllocationError(nManager);
  char *tmpCwd = malloc(sizeof(char) * PATH_MAX);
  rc_al4 = checkAllocationError(tmpCwd);

  int newNManager = 0;
  int newNWorker = 0;
  int type;
  int toSkip;

  if (rc_al != SUCCESS || rc_al2 != SUCCESS || rc_al3 != SUCCESS ||
      rc_al4 != SUCCESS) {
    rc_t = MALLOC_FAILURE;
  } else {
    pthread_mutex_lock(&(sharedResources->mutex));
    // TODO... check strcpy errors with the special function created just for this purpose
    strcpy(tmpCwd, sharedResources->cwd);
    pthread_mutex_unlock(&(sharedResources->mutex));
  }

  while (rc_t == SUCCESS) {
    rc_ct = SUCCESS;
    rc_rd = readDirectives(newPath, nManager, nWorker);
    if(rc_rd != SUCCESS){
      rc_t = errorHandler(rc_rd);
    }
    int rc_sc = sscanf(nManager, "%d", &newNManager);
    int rc_sc2 = sscanf(nWorker, "%d", &newNWorker);
    if (rc_sc == 0 || (newNManager == 9 && strcmp(nManager, "9") != 0) ||
        nManager[0] == 0) {
      rc_ct = CAST_FAILURE;
    }
    if (rc_sc2 == 0 || (newNWorker == 9 && strcmp(nWorker, "9") != 0) ||
        nWorker[0] == 0) {
      rc_ct = CAST_FAILURE;
    }

    if (rc_rd == SUCCESS && rc_ct != CAST_FAILURE) {
      pthread_mutex_lock(&(sharedResources->mutex));
      *(sharedResources->nWorker) = newNWorker;
      // TODO... same as above
      strcpy(sharedResources->path, newPath);
      if (*(sharedResources->nManager) != newNManager) {
        rc_cma = changeManagersAmount(sharedResources->managers,
                             *(sharedResources->nManager), newNManager,
                             sharedResources->fileToAssign);
        if(rc_cma != SUCCESS){
          rc_t = errorHandler(rc_cma);
        }
        *(sharedResources->nManager) = newNManager;
      }
      pthread_mutex_unlock(&(sharedResources->mutex));
    } else {
      rc_t = errorHandler(INVALID_SYNTAX_ERROR);
    }
    
    if (rc_ct == SUCCESS) {
      if(strcmp(newPath, "//") == 0){
        pthread_mutex_lock(&(sharedResources->mutex));
        changeWorkerAmount(sharedResources->managers, *(sharedResources->nWorker));
        pthread_mutex_unlock(&(sharedResources->mutex));
      } else {
        type = precomputeAnalyzerInput(tmpCwd, newPath, &rc_an);
        if (rc_an != SUCCESS) {
          if (rc_an != EMPTY_PATH) {
            rc_t = errorHandler(rc_an);
          }
        } else {
          if (type == DIRECTORY) {
            addForwardSlash(newPath);
          }
          if (type == DIRECTORY || type == IS_FILE) {
            toSkip = 0;
            startingNode = NULL;
            pthread_mutex_lock(&(sharedResources->mutex));
            startingNode = precomputeStartingDirectory(
                sharedResources->fs, sharedResources->currentDirectory, newPath,
                &toSkip);
            toAnalyze = newTreeNodeCandidate(startingNode, type, newPath, toSkip);
            if (toAnalyze != NULL) {
              // TODO... check push return code
              push(sharedResources->candidateNode, toAnalyze);
            } else {
              rc_t = errorHandler(MALLOC_FAILURE);
            }
            pthread_mutex_unlock(&(sharedResources->mutex));
          } else {
            if (type == NOT_EXISTING) {
              char *msgInfo = (char *)malloc(sizeof(char) * (PATH_MAX + 300));
              rc_al5 = checkAllocationError(msgInfo);
              if (rc_al5 < 0) {
                rc_t = errorHandler(MALLOC_FAILURE);
              } else {
                sprintf(msgInfo, "The File %s doesn't exist", newPath);
                printInfo(msgInfo);
                free(msgInfo);
              }
            } else {
              rc_t = errorHandler(FILE_TYPE_NOT_RECOGNIZED);
            }
          }
        }
      }
    }
    usleep(500);
  }
  // TODO... debug only
  printf("sono morto in read directives loop: %d\n", rc_t);
  free(newPath);
  free(nWorker);
  free(nManager);
  free(tmpCwd);
  kill(getpid(), SIGKILL);
}

int readDirectives(char *path, char *nManager, char *nWorker) {
  char readBuffer[2] = "a";
  // TODO... I don't know the meaning of the returning parameter... maybe could be void function?
  // TODO... in any case we need to adjust the comment if we change this
  int rc_t = SUCCESS;
  int counter = 0;
  do {
    int rc = readChar(READ_CHANNEL, readBuffer);
    path[counter++] = readBuffer[0];
  } while (readBuffer[0] != '\0' && readBuffer[0] != '\n');
  path[counter] = '\0';
  if (path[strlen(path) - 1] == '\n') {
    path[strlen(path) - 1] = '\0';
  }

  counter = 0;
  do {
    int rc = readChar(READ_CHANNEL, readBuffer);
    nManager[counter++] = readBuffer[0];
  } while (readBuffer[0] != '\0' && readBuffer[0] != '\n');
  nManager[counter - 1] = '\0';

  counter = 0;
  do {
    int rc = readChar(READ_CHANNEL, readBuffer);
    nWorker[counter++] = readBuffer[0];
  } while (readBuffer[0] != '\0' && readBuffer[0] != '\n');
  nWorker[counter - 1] = '\0';
  
  // TODO... debug only
  /*
  printf("Path: %s\n", path);
  printf("Manager: %s\n", nManager);
  printf("Worker: %s\n", nWorker);
  */
  return rc_t;
}

int changeManagersAmount(PriorityQueue managers, const int currentManagers,
                         const int newManagers, List fileToAssign) {
  int rc_t;
  int delta;
  if (currentManagers > 0)
    delta = newManagers - currentManagers;
  else
    delta = newManagers;

  if (delta > 0) {
    rc_t = addManagers(managers, delta);
  } else {
    rc_t = removeManagers(managers, -delta, fileToAssign);
  }

  return rc_t;
}

int addManagers(PriorityQueue managers, int amount) {
  int rc_t = SUCCESS;
  int rc_en = SUCCESS;
  int i = 0;
  int toParent[2];
  int toChild[2];
  int rc_p1;
  int rc_p2;
  int rc_nm = SUCCESS;
  if (managers != NULL) {
    for (i = 0; i < amount && rc_t == SUCCESS; i++) {
      Manager manager = newManager();
      if (manager == NULL)
        rc_t = NEW_MANAGER_FAILURE;
      else {
        rc_en = pushPriorityQueue(managers, manager->filesInExecution->size,
                                  (void *)manager);
        if (rc_en != SUCCESS)
          rc_t = MALLOC_FAILURE;
        else {
          rc_p1 = createUnidirPipe(toParent);
          rc_p2 = createUnidirPipe(toChild);
          int managerPid = fork();
          if(rc_p1 != SUCCESS || rc_p2 != SUCCESS){
            rc_t = PIPE_FAILURE;
          }else{
            if (managerPid > 0) {
              int rc_pp = parentInitExecPipe(toParent, toChild);
              int rc_fc = fcntl(toParent[READ_CHANNEL], F_SETFL, O_NONBLOCK);
              if (rc_pp == -1 || rc_fc == -1)
                rc_t = PIPE_FAILURE;
              else {
                manager->m_pid = managerPid;
                manager->pipe[0] = toParent[READ_CHANNEL];
                manager->pipe[1] = toChild[WRITE_CHANNEL];
              }
            } else {
              managerInitPipe(toParent, toChild);
              rc_nm = execlp("./manager", "./manager", NULL);
              if(rc_nm != SUCCESS){
                rc_t = errorHandler(rc_nm);
              }
              kill(getpid(), SIGKILL);
            }
          }
        }
      }
    }
  } else {
    rc_t = MALLOC_FAILURE;
  }
  return rc_t;
}

int managerInitPipe(const int toParent[], const int toChild[]) {
  int rc_t = 0;
  int rc_cl = closeDescriptor(toChild[WRITE_CHANNEL]);
  int rc_cl2 = closeDescriptor(toParent[READ_CHANNEL]);
  int rc_du = createDup(toChild[READ_CHANNEL], 0);
  int rc_du2 = createDup(toParent[WRITE_CHANNEL], 1);
  int rc_cl3 = closeDescriptor(toChild[READ_CHANNEL]);
  int rc_cl4 = closeDescriptor(toParent[WRITE_CHANNEL]);

  if (rc_cl == -1 || rc_cl2 == -1 || rc_cl3 == -1 || rc_cl4 == -1 ||
      rc_du == -1 || rc_du2 == -1) {
    rc_t = PIPE_FAILURE;
  }

  return rc_t;
}

Manager newManager() {
  Manager manager = (Manager)malloc(sizeof(struct Manager));
  int rc_ca = checkAllocationError(manager);
  if (rc_ca == SUCCESS) {
    manager->filesInExecution = newList();
    if (manager->filesInExecution == NULL) {
      free(manager->filesInExecution);
      free(manager);
      manager = NULL;
    } else {
      manager->pipe = (int *)malloc(sizeof(int) * 2);
      rc_ca = checkAllocationError(manager->pipe);
      if (rc_ca != SUCCESS) {
        free(manager->filesInExecution);
        free(manager);
        manager = NULL;
      }
    }
  }
  return manager;
}

int removeManagers(PriorityQueue managers, int amount, List fileToAssign) {
  int rc_t = SUCCESS;
  int rc_em = SUCCESS;
  int rc_im = SUCCESS;
  int rc_pu = SUCCESS;
  int rc_sw = SUCCESS;
  int managerSize = managers->len;
  PriorityQueue tmpManagers = newPriorityQueue();
  if(tmpManagers == NULL){
    rc_t = MALLOC_FAILURE;
  }
  while (rc_t == SUCCESS && managerSize != 0) {
    Manager m = popPriorityQueue(managers);
    if(m != NULL){
      if (amount != 0) {
        if (m != NULL) {
          rc_em = endManager(m, fileToAssign);
          if(rc_em != SUCCESS){
            rc_t = errorHandler(rc_em);
          }
          kill(m->m_pid, SIGKILL);
          destroyManager((void *)m);
          amount--;
        } else {
          rc_t = REMOVE_MANAGER_FAILURE;
        }
      } else {
        rc_em = endManager(m, fileToAssign);
        if(rc_em != SUCCESS){
          rc_t = errorHandler(rc_em);
        }
        rc_im = informManager(m);
        if(rc_t == SUCCESS && rc_im != SUCCESS){
          rc_t = errorHandler(rc_im);
        }
        rc_pu = pushPriorityQueue(tmpManagers, m->filesInExecution->size, (void *)m);
        if(rc_t == SUCCESS && rc_im != rc_pu){
          rc_t = errorHandler(MALLOC_FAILURE);
        }
      }
    } else {
      rc_t = UNEXPECTED_PRIORITY_QUEUE_FAILURE;
    }
    managerSize--;
  }
  // TODO... check for swap errors
  rc_sw = swapPriorityQueue(managers, tmpManagers);
  destroyPriorityQueue(tmpManagers, free);
  if(rc_sw != SUCCESS){
    rc_t = errorHandler(UNEXPECTED_PRIORITY_QUEUE_FAILURE);
  }
  return rc_t;
}

int endManager(Manager m, List fileToAssign) {
  int rc_t = SUCCESS;
  int rc_sw = SUCCESS;
  int rc_cnt = SUCCESS;
  if (fileToAssign != NULL) {
    if (isEmptyList(fileToAssign) == EMPTY) {
      rc_sw = swap(fileToAssign, m->filesInExecution);
    } else {
      rc_cnt = concat(fileToAssign, m->filesInExecution);
    }
    if(rc_sw != SUCCESS || rc_cnt != SUCCESS){
      rc_t = UNEXPECTED_LIST_ERROR;
    }
  } else {
    rc_t = MALLOC_FAILURE;
  }
  return rc_t;
}

void destroyManager(void *data) {
  Manager manager = (Manager)data;
  if (manager->pipe != NULL) {
    closeDescriptor(manager->pipe[0]);
    closeDescriptor(manager->pipe[1]);
  }
  free(manager->pipe);
  free(manager->filesInExecution);
  free(manager);
}

int informManager(Manager manager) {
  int rc_t = SUCCESS;
  int *fd = manager->pipe;
  char *stopMsg = malloc(sizeof(char) * CONTROL_WORD_LEN);
  int rc_al = checkAllocationError(stopMsg);
  int rc_ss = sprintf(stopMsg, "%s", CONTROL_STOP);
  if (fd != NULL) {
    if (rc_al == SUCCESS) {
      if (rc_ss >= 0) {
        int rc_wr = writeDescriptor(fd[WRITE_CHANNEL], stopMsg);
        int rc_wr2 = writeDescriptor(fd[WRITE_CHANNEL], stopMsg);
        if (rc_wr < SUCCESS || rc_wr2 < SUCCESS) {
          rc_t = SEND_FAILURE;
        }
      } else {
        // TODO should it die here?
        rc_t = SPRINTF_FAILURE;
      }
    } else {
      rc_t = MALLOC_FAILURE;
    }
  } else {
    rc_t = SEND_FAILURE;
  }
  free(stopMsg);
  return rc_t;
}

int precomputeAnalyzerInput(char *cwd, char *path, int *msg) {
  *msg = SUCCESS;
  int type = NOT_EXISTING;
  int isAbsolutePath = RELATIVE;
  if (path[0] == 0) {
    *msg = EMPTY_PATH;
  } else {
    if (path[0] == '/') {
      isAbsolutePath = ABSOLUTE;
    }
    *msg = compactPath(path, cwd, isAbsolutePath);
    if (*msg == SUCCESS) {
      type = checkFileType(path);
    }
  }
  return type;
}

int compactPath(char *path, char *cwd, int isAbsolute) {
  int rc_t = SUCCESS;
  if (isAbsolute == ABSOLUTE) {
    char *argv[4] = {"realpath", "-m", path, NULL};
    rc_t = callBashProcess(path, argv);
  } else {
    char *argv[7] = {"realpath", "-m", "--relative-to", cwd, path, NULL};
    rc_t = callBashProcess(path, argv);
  }
  return rc_t;
}

int callBashProcess(char *dst, char *argv[]) {
  int rc_t = SUCCESS;
  int rc_rb;
  int rc_wb;
  int fd[2];
  int rc_fd = createUnidirPipe(fd);
  int rc_fc = fcntl(fd[READ_CHANNEL], F_SETFL, O_NONBLOCK);
  if (rc_fd == SUCCESS && rc_fc == SUCCESS) {
    int f = fork();
    if (f > 0) {
      rc_rb = readBashLines(fd, dst, f);
    } else if (f == 0) {
      rc_wb = writeBashProcess(fd, argv);
    } else {
      rc_t = FORK_FAILURE;
    }
    if (rc_t != SUCCESS) {
      if (rc_rb != SUCCESS) {
        rc_t = READ_BASH_FAILURE;
      } else if (rc_wb != SUCCESS) {
        rc_t = WRITE_BASH_FAILURE;
      }
    }
  } else {
    rc_t = PIPE_FAILURE;
  }

  return rc_t;
}

int writeBashProcess(int *fd, char *argv[]) {
  int rc_t = SUCCESS;
  int rc_cds_1 = closeDescriptor(fd[READ_CHANNEL]);
  int rc_cdp = createDup(fd[WRITE_CHANNEL], 1);
  int rc_cds_2 = closeDescriptor(fd[WRITE_CHANNEL]);
  if(rc_cds_1 != SUCCESS || rc_cdp != SUCCESS || rc_cds_2 != SUCCESS){
    rc_t = PIPE_FAILURE;
  }
  int rc_ex = execvp(argv[0], argv);
  if (rc_t == SUCCESS && rc_ex != SUCCESS) {
    char *msgErr = (char *)malloc(sizeof(char) * 1000);
    int rc_ca = checkAllocationError(msgErr);
    if (rc_ca < 0) {
      rc_t = errorHandler(MALLOC_FAILURE);
    } else {
      rc_t = errorHandler(BASH_FAILURE);
      free(msgErr);
    }
    kill(getpid(), SIGKILL);
  }
  return rc_t;
}

//! SONO QUI PER FARE LE COSE DEI ERRORHANDLER 
int readBashLines(int *fd, char *dst, int childPid) {
  // TODO... also this return code is useless at the moment
  int rc_t = SUCCESS;
  int rc_cds_1;
  int rc_cds_2;
  int bytesRead = 1;
  char charRead = 'a';
  int counter = 0;
  int firstCycleAfterDeath = 1;

  rc_cds_1 = closeDescriptor(fd[WRITE_CHANNEL]);

  while (bytesRead > 0 || (kill(childPid, 0) == 0) ||
         firstCycleAfterDeath >= 0) {
    bytesRead = read(fd[READ_CHANNEL], &charRead, 1);
    if (bytesRead > 0) {
      if (charRead != '\n') {
        dst[counter] = charRead;
        counter++;
      }
    }
    if (kill(childPid, 0) != 0)
      firstCycleAfterDeath--;
  }
  dst[counter] = '\0';
  // TODO... Deal with pipe
  rc_cds_2 = closeDescriptor(fd[READ_CHANNEL]);
  //! Missing error handling
  if (rc_cds_1 != SUCCESS) {
    // TODO... maybe better errors?
    printError("Allllllllllora, non funziona la close descriptor in fd write "
               "in read bash line");
  } else if (rc_cds_2 != SUCCESS) {
    // TODO... maybe better errors?
    printError("Allllllllllora, non funziona la close descriptor in fd read in "
               "read bash line");
  }
  return rc_t;
}

int checkFileType(char *name) {
  int rc_al = SUCCESS;
  int rc_sp = SUCCESS;
  int rc_ss = SUCCESS;
  int rc_t = SUCCESS;
  char *command = (char *)malloc(sizeof(char) * (PATH_MAX * 2 + 150));
  rc_al = checkAllocationError(command);
  if (rc_al != SUCCESS) {
    rc_t = MALLOC_FAILURE;
  } else {
    rc_sp = sprintf(
        command,
        "([ -f '%s' ] && echo '0') || ([ -d '%s' ] && echo '1') || echo '2'",
        name, name);
    if (rc_sp > 0) {
      char *argv[4] = {"sh", "-c", command, NULL};
      char type[2];
      rc_t = callBashProcess(type, argv);
      if (rc_t == SUCCESS) {
        rc_ss = sscanf(type, "%d", &rc_t);
        if (rc_ss <= 0) {
          //! ????
          rc_t = errorHandler(SSCANF_FAILURE);
        }
      }
    } else {
      rc_t = errorHandler(SPRINTF_FAILURE);
    }
    free(command);
  }
  // Handle error
  return rc_t;
}

void addForwardSlash(char *path) {
  int stringLength = strlen(path);
  if (path[stringLength - 1] != '/') {
    path[stringLength] = '/';
    path[stringLength + 1] = '\0';
  }
}

TreeNode precomputeStartingDirectory(Tree fs, TreeNode currentDirectory,
                                     char *path, int *toSkip) {
  TreeNode startingNode = NULL;
  *toSkip = 0;
  if (path != NULL && fs != NULL &&
      currentDirectory != NULL) { // Verify if everything is ok
    if (path[0] == '/') { // if my path start with / then i'm talking about an
                          // Absolute Path
      startingNode = getRoot(fs);
      *toSkip = 1;
    } else if (path[0] == '.') { // if my path starts with a dot...
      if (path[1] == '.') {      // and is followed by a dot...
        if (path[2] == '/') {    // then I need to go up but...
          startingNode = goUp(fs, currentDirectory, path, toSkip);
        } else { // I didn't even know it could be possible to create a folder
                 // called .......something
          startingNode = currentDirectory;
        }
      } else {
        if (path[1] == '/') { // If the first dot is followed by a slash then
                              // I'm talking about the folder I'm in
          *toSkip = 2;
        } // Otherwise I'm talking about an hidden folder
        startingNode = currentDirectory;
      }
    } else { // In any other case I'm talking about a folder or a file inside my
             // current folder
      startingNode = currentDirectory;
    }
  }
  return startingNode;
}

TreeNode goUp(Tree fs, TreeNode currentDirectory, char *path, int *toSkip) {
  TreeNode startingNode = NULL;
  int keepGoing = 0;
  int counter = 3;
  startingNode = currentDirectory->parent;
  while (path[counter] == '.' &&
         keepGoing == 0) { // I still need to check if I need to go up more
    if (path[counter + 1] == '.') {
      if (path[counter + 2] == '/') {
        counter += 3;
        startingNode = startingNode->parent;
        if (startingNode == NULL) {
          keepGoing = -1;
          printError("Something went terribly wrong in the path analysis so "
                     "we'll attach it to the root");
          startingNode = getRoot(fs);
        }
      }
    } else { // I'm arrived at the rigth level
      keepGoing = -1;
    }
  }
  *toSkip = counter;
  return startingNode;
}

TreeNodeCandidate newTreeNodeCandidate(TreeNode startingNode, int typeOfFile,
                                       char *path, int characterToSkip) {
  TreeNodeCandidate candidate =
      (TreeNodeCandidate)malloc(sizeof(struct TreeNodeCandidate));
  int rc_al = checkAllocationError(candidate);
  if (rc_al == SUCCESS) {
    candidate->path = (char *)malloc(sizeof(char) * PATH_MAX);
    int rc_al2 = checkAllocationError(candidate->path);
    if (rc_al2 == SUCCESS) {
      candidate->startingNode = startingNode;
      candidate->type = typeOfFile;
      //TODO... handle with strcpy function for any errors
      strcpy(candidate->path, path);
      candidate->toSkip = characterToSkip;
    } else {
      free(path);
      free(candidate);
    }
  } else {
    free(candidate);
  }
  return candidate;
}

void destroyTreeNodeCandidate(void *data) {
  TreeNodeCandidate candidate = (TreeNodeCandidate)data;
  free(candidate->path);
  free(candidate);
}

//TODO... debug only, does it really need a comment?
void printCandidateNode(void *data) {
  TreeNodeCandidate candidate = (TreeNodeCandidate)data;
  printf("Starting node: %p, path: %s, tof: %d, toSkipper: %d\n",
         candidate->startingNode, candidate->path, candidate->type,
         candidate->toSkip);
}

/**
 * Function that handle the multiple instance of .. inside the path in order to get at the highest level
 * 
 * args:
 *    TreeNode startingNode: the starting directory where the search inside the tree will start
 *    int typeOfFile: the type of the file which will be analyzed (ISFILE or DIRECTORY)
 *    char *path: the valid path of a file or a folder which is passed in input 
 *    int characterToSkip:  number of characters to skip because are not part of the file or folder name and
 *      only useful for the navigation inside the tree
 *   
 *
 * returns
 *    A TreeNodeCandidate which will be put later on inside a list in order to be analyzed
 */





//TODO... remove this global list of elements
List finished = NULL;

int respawnManager(PriorityQueue managers, Manager dead, List fileToAssign) {
  int rc_t = SUCCESS;
  rc_t = endManager(dead, fileToAssign);
  if (rc_t < SUCCESS) {
    printInfo("One or more managers are dead\n");
  } else {
    rc_t = addManagers(managers, 1);
  }
  destroyManager((void *)dead);
  return rc_t;
}

void toStringTable(long long unsigned int *table) {
  int i;
  for (i = 0; i < NCHAR_TABLE; i++) {
    printf("%llu ", table[i]);
  }
  printf("\n");
}

/**
 *  File info toString
 */
void pasta(void *data) {
  fprintf(stderr, "File : %s name %s isFolder %d\n",
          ((FileInfo)((TreeNode)data)->data)->path,
          ((FileInfo)((TreeNode)data)->data)->name,
          ((FileInfo)((TreeNode)data)->data)->isDirectory);
}

void pasta2(void *data) {
  printf("File : %s name %s isFolder %d\n", ((FileInfo)data)->path,
         ((FileInfo)data)->name, ((FileInfo)data)->isDirectory);
}

void printRequestedNodes(void *data) {
  TreeNode node = (TreeNode)data;
  FileInfo reqFile = (FileInfo) node->data;
  pasta2(reqFile);
}

/**
 * Manager toString function
 */
void toStringManager(void *data) {
  Manager manager = (Manager)data;
  printf("size: %d\n", manager->filesInExecution->size);
  printList(manager->filesInExecution, pasta);
}

int insertAndSchedule(TreeNode startingNode, List filesToAssign,
                      char *relativePath, char *completePath, List requestedFiles) {
  int rc_t = SUCCESS;
  int rc_af = SUCCESS;
  int rc_en = SUCCESS;
  TreeNode toSchedule = NULL;
  // LOCK
  // pasta((void *)currentDirectory);
  toSchedule =
      performInsert(relativePath, completePath, startingNode, IS_FILE, &rc_t);
  // printf("qui mi rompo\n");
  // MAYBE RELEASE THE LOCK???
  // TODO... Handle with the errorHandler (maybe write a message just to inform the user that the file was already passed) 
  if (rc_t == SUCCESS || rc_t == ALREADY_INSERTED) {
    rc_en = enqueue(filesToAssign, (void *)toSchedule);

    // rc_af = assignFileToManager(toSchedule, managers, nManager, nWorker);
    if (rc_en != SUCCESS) {
      printError("Error in assign file to manager");
      rc_t = rc_en;
    }
    // HANDLE THE MULTIPLE ERROR WITH THE ERROR HANDLER
  }
  return rc_t;
  // UNLOCK
}

/**
 * Function that schedule a file
 */
int scheduleFile(TreeNode toSchedule, Manager manager) {
  int rc_t = SUCCESS;
  if (toSchedule != NULL) {
    rc_t = checkAllocationError(manager->filesInExecution);
    if (rc_t == SUCCESS) {
      rc_t = enqueue(manager->filesInExecution, (void *)toSchedule);
    } else {
      rc_t = MALLOC_FAILURE;
    }
  } else {
    rc_t = FILE_NULL_POINTER;
  }
  return rc_t;
}

int parentInitExecPipe(const int toParent[], const int toChild[]) {
  int rc_t = 0;
  int rc_cl = closeDescriptor(toParent[WRITE_CHANNEL]);
  int rc_cl2 = closeDescriptor(toChild[READ_CHANNEL]);

  if (rc_cl == -1 || rc_cl2 == -1) {
    rc_t = PIPE_FAILURE;
  }
  return rc_t;
}

int sendFile(Manager manager, TreeNode file, List filesToAssign,
             int currentWorker) {
  int rc_t = SUCCESS;
  int rc_po = pop(filesToAssign);
  int rc_en = enqueue(manager->filesInExecution, file);

  if (rc_po == SUCCESS && rc_en == SUCCESS) {
    int *pipe = manager->pipe;
    char *nworkers = malloc((INT_MAX_LEN + 1) * sizeof(char));
    int rc_al = checkAllocationError(nworkers);
    if (rc_al < SUCCESS)
      rc_t = MALLOC_FAILURE;
    else {
      int rc_sp1 = sprintf(nworkers, "%d", currentWorker);
      if (rc_sp1 > 0) {
        int rc_wr =
            writeDescriptor(pipe[WRITE_CHANNEL], ((FileInfo)file->data)->path);
        int rc_wr_2 = writeDescriptor(pipe[WRITE_CHANNEL], nworkers);
        if (rc_wr < SUCCESS || rc_wr_2 < SUCCESS)
          rc_t = SEND_FAILURE;
      } else
        rc_t = SEND_FAILURE;
      free(nworkers);
    }
  } else
    rc_t = MALLOC_FAILURE;
  return rc_t;
}

int isManagerAlive(Manager m) {
  int rc_t = SUCCESS;
  int returnCode = kill(m->m_pid, 0);
  if (returnCode != 0) {
    rc_t = DEAD_PROCESS;
  }
  return rc_t;
}

int rescheduleFiles(List toDestroy, List toReschedule) {
  int rc_t = SUCCESS;
  if (isEmptyList(toReschedule) == EMPTY) {
    if (isEmptyList(toDestroy) != EMPTY) {
      rc_t = swap(toDestroy, toReschedule);
      if(rc_t != SUCCESS){
        rc_t = UNEXPECTED_LIST_ERROR;
      }
    }
  } else {
    TreeNode file = NULL;
    while (isEmptyList(toDestroy) == NOT_EMPTY && rc_t == SUCCESS) {
      file = (TreeNode)front(toDestroy);
      rc_t = pop(toDestroy);
      if (rc_t == SUCCESS) {
        rc_t = enqueue(toReschedule, (void *)file);
        if(rc_t != SUCCESS){
          rc_t = errorHandler(MALLOC_FAILURE);
        }
      } else {
        rc_t = errorHandler(UNEXPECTED_LIST_ERROR);
      }
    }
  }
  return rc_t;
}

void *sendFileLoop(void *ptr) {
  sharedResourcesAnalyzer_t *sharedResources = (sharedResourcesAnalyzer_t *)ptr;
  Node node = NULL;
  TreeNode file = NULL;
  FileInfo info = NULL;
  int rc_t = SUCCESS;
  int rc_mfs = SUCCESS;
  int rc_ss = SUCCESS;
  int rc_sw = SUCCESS;
  int rc_em = SUCCESS;
  int rc_rm = SUCCESS;
  int rc_pu = SUCCESS;
  int rc_dn = SUCCESS;
  int alreadyPushed = FAILURE;
  int nManager = 0;
  int isAliveM = 0;
  int found = 0;
  int counter = 0;
  unsigned long long charCounter = 0;
  int *pipe = NULL;
  char *path = (char *)malloc(sizeof(char) * PATH_MAX);
  char *number = (char *)malloc(sizeof(char) * PATH_MAX);
  char charRead = 'a';
  int numbersToRead = NCHAR_TABLE;
  int bytesRead = 1;
  int insertCounter = 0;
  //! DEBUG ONLY
  int counterFiles = 0;
  char *controlWord = (char *)malloc(sizeof(char) * CONTROL_WORD_LEN);
  Manager manager = NULL;
  finished = newList();
  PriorityQueue tmpManagers = newPriorityQueue();
  int rc_ca = checkAllocationError(path);
  int rc_ca2 = checkAllocationError(number);
  int rc_ca3 = checkAllocationError(controlWord);
  if (tmpManagers == NULL || rc_ca != SUCCESS || rc_ca2 != SUCCESS ||
      rc_ca3 != SUCCESS) {
    rc_t = errorHandler(MALLOC_FAILURE);
  }
  unsigned long long accumulator = 0;
  while (rc_t == SUCCESS) {
    pthread_mutex_lock(&(sharedResources->mutex));
    nManager = *(sharedResources->nManager);
    while (nManager > 0 && rc_t == SUCCESS) {
      manager = popPriorityQueue(sharedResources->managers);
      if (manager != NULL) {
        isAliveM = isManagerAlive(manager);
        if (isAliveM == SUCCESS) {
          pipe = manager->pipe;
          bytesRead = read(pipe[READ_CHANNEL], &charRead, 1);
          if (bytesRead > 0) {
            while (charRead != 0 && isManagerAlive(manager) == SUCCESS) {
              if (bytesRead > 0) {
                path[counter] = charRead;
                counter++;
              }
              bytesRead = read(pipe[READ_CHANNEL], &charRead, 1);
            }
            path[counter] = 0;
            node = manager->filesInExecution->head;
            found = 0;
            while (node != NULL && found == 0 && rc_t == SUCCESS) {
              file = (TreeNode)node->data;
              if (file != NULL) {
                info = (FileInfo)file->data;
                if (info != NULL) {
                  if (strcmp(path, info->path) == 0) {
                    found = 1;
                  } else {
                    node = node->next;
                  }
                } else {
                  rc_t = errorHandler(NON_EXISTING_FILE);
                }
              } else {
                rc_t = errorHandler(NON_EXISTING_FILE);
              }
            }
            if (rc_t != SUCCESS) {
              rc_t = errorHandler(FILE_NOT_FOUND);
            } else {
              insertCounter = 0;
              numbersToRead = NCHAR_TABLE;
              while (numbersToRead > 0 && isManagerAlive(manager) == SUCCESS) {
                counter = 0;
                charRead = 'a';
                while (charRead != 0 && isManagerAlive(manager) == SUCCESS) {
                  bytesRead = read(pipe[READ_CHANNEL], &charRead, 1);
                  if (bytesRead > 0) {
                    number[counter] = charRead;
                    counter++;
                  }
                }
                number[counter] = 0;
                rc_ss = sscanf(number, "%llu", &charCounter);
                if (rc_ss > 0) {
                  if(found == 1){
                    if(info->isRequested == SUCCESS){
                      sharedResources->requestedFilesTable[insertCounter] -= info->fileTable[insertCounter];
                      sharedResources->requestedFilesTable[insertCounter] += charCounter;
                    }
                    info->fileTable[insertCounter++] = charCounter;
                  }
                } else {
                  //!CHE CRISTO DI ERRORE FARE? Morire o vivere?
                  rc_t = errorHandler(SSCANF_FAILURE);
                }
                number[0] = 0;
                numbersToRead--;
              }
              counter = 0;
              int stopRead = 0;
              if (isManagerAlive(manager) == SUCCESS) {
                while (isManagerAlive(manager) == SUCCESS && stopRead == 0) {
                  bytesRead = read(pipe[READ_CHANNEL], controlWord, CONTROL_WORD_LEN);
                  int i = 0;
                  if (bytesRead > 0) {
                    if (strcmp(controlWord, CONTROL_DONE) == 0) {
                      if(found == 1){
                        counterFiles++;
                        // printf("path: %s, size: %d\n", path, counterFiles);
                        rc_dn = detachNodeFromList(manager->filesInExecution, node);
                        if(info->isRequested == SUCCESS){
                          sharedResources->sendChanges = SUCCESS;
                        }
                        if(rc_dn != SUCCESS){
                          rc_t = errorHandler(rc_dn);
                        }
                      }
                      rc_pu = push(finished, info);
                      if(rc_pu != SUCCESS){
                        rc_t = errorHandler(MALLOC_FAILURE);
                      }
                    } else if (strcmp(controlWord, CONTROL_UNDONE) == 0) {
                      if(found == 1){
                        if(info->isRequested == SUCCESS){
                          sharedResources->sendChanges = SUCCESS;
                        }
                      }
                    } else {
                      rc_t = errorHandler(ANALYZER_MANAGER_MISUNDERSTANDING);
                    }
                    stopRead = 1;
                  }
                }
              } else {
                //printf("MUOIO pt 1\n");
                rc_rm = respawnManager(tmpManagers, manager,
                                       sharedResources->fileToAssign);
                //printf("RESPAWNO pt 1\n");
                alreadyPushed = SUCCESS;
                rc_t = errorHandler(rc_rm);
              }
            }
          }
        } else {
          //printf("MUOIO pt 2\n");
          rc_rm = respawnManager(tmpManagers, manager,
                                 sharedResources->fileToAssign);
          //printf("RESPAWNO pt 2\n");
          alreadyPushed = SUCCESS;
          rc_t = errorHandler(rc_rm);
        }
        if (alreadyPushed == FAILURE) {
          rc_pu = pushPriorityQueue(tmpManagers, manager->filesInExecution->size,
                            manager);
          if(rc_pu != SUCCESS){
            rc_t = errorHandler(MALLOC_FAILURE);
          }
        } else {
          alreadyPushed = FAILURE;
        }
      } else {
        rc_t = errorHandler(NULL_POINTER);
      }
      nManager--;
    }
    rc_sw = swapPriorityQueue(sharedResources->managers, tmpManagers);
    if (rc_sw != SUCCESS) {
      rc_t = errorHandler(UNEXPECTED_PRIORITY_QUEUE_FAILURE);
    }
    if (isEmptyList(sharedResources->fileToAssign) == NOT_EMPTY) {
      rc_mfs = manageFileToSend(sharedResources->managers,
                                *(sharedResources->nWorker),
                                sharedResources->fileToAssign);
      if(rc_mfs != SUCCESS){
        errorHandler(rc_mfs);
      }
    }
    pthread_mutex_unlock(&(sharedResources->mutex));
    usleep(500);
  }
  printf("ATTENZIONE: sto a mori' qui %d\n", rc_t);
  kill(getpid(), SIGKILL);
}

/*
void sighandle_print(int sig) {
  // printList(finished, pasta2);
  int fd = open("/tmp/conteggio.txt", O_WRONLY);
  // printf("ACCUMULATORE %llu\n", accumulator);
  printf("Scorro tutto\n");
  while (isEmptyList(finished) == NOT_EMPTY) {
    // printf("SONO UN FIGLIO DI TROIA\n");
    FileInfo finfo = front(finished);
    pop(finished);
    //! nome file \n tabella tutta su una riga separata da spazi \n
    unsigned long long counterSchifo = 0;
    char *stringa = calloc(PATH_MAX + 1, sizeof(char));
    strcpy(stringa, finfo->path);
    write(fd, stringa, strlen(stringa));
    write(fd, "\n", 1);
    int i = 0;
    for (i = 0; i < NCHAR_TABLE; i++) {
      counterSchifo = finfo->fileTable[i];
      sprintf(stringa, "%llu ", counterSchifo);
      write(fd, stringa, strlen(stringa));
    }
    write(fd, "\n", 1);
    free(stringa);
  }
  close(fd);
  printf("Ho finito di scrivere\n");
  signal(SIGINT, SIG_DFL);
}
*/

int manageFileToSend(PriorityQueue managers, int currentWorker,
                     List filesToAssign) {
  int rc_t = SUCCESS;
  int rc_sf = SUCCESS;
  int rc_po = SUCCESS;
  int rc_pu = SUCCESS;
  int rc_rm = SUCCESS;
  int isAliveM = SUCCESS;
  if (managers != NULL) {
    Manager m = (Manager)popPriorityQueue(managers);
    if (m != NULL) {
      isAliveM = isManagerAlive(m);
      if (isAliveM == SUCCESS) {
        if (isEmptyList(filesToAssign) == NOT_EMPTY) {
          TreeNode file = front(filesToAssign);
          if (file != NULL) {
            rc_sf = sendFile(m, file, filesToAssign, currentWorker);
            rc_pu = pushPriorityQueue(managers, m->filesInExecution->size,
                                      (void *)m);
            if (rc_pu < SUCCESS) {
              rc_t = MALLOC_FAILURE;
            } else {
              rc_t = rc_sf;
            }
          } else {
            rc_t = ASSIGNWORK_FILE_FAILURE;
          }
        }
      } else {
        rc_rm = respawnManager(managers, m, filesToAssign);
        rc_t = DEAD_PROCESS;
      }
    }
  } else {
    rc_t = NULL_POINTER;
  }
  return rc_t;
}

int getCwd(char *dst) {
  int rc_cwd = SUCCESS;
  if (getcwd(dst, PATH_MAX) == NULL) {
    rc_cwd = CWD_FAILURE;
  }
  return rc_cwd;
}

int spawnFindProcess(char *compactedPath, int *fd, int *childPid) {
  int rc_t = SUCCESS;
  int rc_cds_1 = SUCCESS;
  int rc_cds_2 = SUCCESS;
  int rc_cdp = SUCCESS;
  int rc_exlp = SUCCESS;
  int rc_fd = createUnidirPipe(fd);
  int rc_fc = fcntl(fd[READ_CHANNEL], F_SETFL, O_NONBLOCK);
  if (rc_fd == SUCCESS && rc_fc != -1) {
    int f = fork();
    if (f > 0) {
      // TODO... Deal with pipe
      rc_cds_1 = closeDescriptor(fd[WRITE_CHANNEL]);
      if(rc_cds_1 != SUCCESS){
        rc_t = PIPE_FAILURE;
      }
      *childPid = f;
    } else if (f == 0) {
      // TODO... Deal with pipe
      rc_cds_1 = closeDescriptor(fd[READ_CHANNEL]);
      rc_cdp = createDup(fd[WRITE_CHANNEL], 1);
      rc_cds_2 = closeDescriptor(fd[WRITE_CHANNEL]);
      rc_exlp = execlp("find", "find", compactedPath, "-mindepth", "1",
                       "-readable", "-type", "f", NULL);
      if(rc_cds_1 != SUCCESS || rc_cdp != SUCCESS || rc_cds_2 != SUCCESS){
        rc_t = errorHandler(PIPE_FAILURE);
      } else if(rc_exlp != SUCCESS){
        rc_t = errorHandler(FIND_FAILURE);
      }
      kill(getpid(), SIGKILL);
    } else {
      rc_t = FORK_FAILURE;
    }
  } else {
    rc_t = PIPE_FAILURE;
  }
  return rc_t;
}

void skipPath(char *path, char *relativePath, int toSkip) {
  int skipped = 0;
  int counter = 0;
  while (path[skipped] != 0) {
    if (skipped >= toSkip) {
      relativePath[counter] = path[skipped];
      counter++;
      skipped++;
    } else {
      skipped++;
    }
  }
  relativePath[counter] = 0;
}

void *fileManageLoop(void *ptr) {
  sharedResourcesAnalyzer_t *sharedResources = (sharedResourcesAnalyzer_t *)ptr;
  TreeNode startingNode = NULL;
  TreeNode toSchedule = NULL;
  TreeNodeCandidate candidate;

  int rc_t = SUCCESS;
  int rc_al = SUCCESS;
  int rc_en = SUCCESS;
  int rc_find = SUCCESS;
  int rc_ia = SUCCESS;
  int rc_pi = SUCCESS;

  char charRead = 'a';
  int skipped = 0;
  int bytesRead = 1;
  int type;
  int fd[2];
  int childPid;
  int counter = 0;
  int readFlag = FAILURE;
  int insertFlag = FAILURE;

  char *relativePath = malloc(PATH_MAX * sizeof(char));
  rc_al = checkAllocationError(relativePath);

  if (rc_al != SUCCESS) {
    rc_t = errorHandler(MALLOC_FAILURE);
  }

  while (rc_t == SUCCESS) {
    if (readFlag != SUCCESS) {
      pthread_mutex_lock(&(sharedResources->mutex));
      if (isEmptyList(sharedResources->candidateNode) == NOT_EMPTY) {
        candidate = (TreeNodeCandidate)front(sharedResources->candidateNode);
        int rt_po = pop(sharedResources->candidateNode);
        if (candidate == NULL || rt_po < SUCCESS) {
          rc_t = errorHandler(UNEXPECTED_LIST_ERROR);
        } else {
          //printf("Setto insertFlag a true dovuto a %s, type %d, skip %d\n",
          //       candidate->path, candidate->type, candidate->toSkip);
          insertFlag = SUCCESS;
        }
      }
      pthread_mutex_unlock(&(sharedResources->mutex));
    } else {
      insertFlag = FAILURE;
    }
    // Parte di inserimento
    if (insertFlag == SUCCESS) {
      //printf("Precompute ok!!\n");
      childPid = 0;
      if (candidate->type == DIRECTORY) {
        rc_find = spawnFindProcess(candidate->path, fd, &childPid);
        //printf("Lo ho spawnato: %d - %d, %d\n", fd[READ_CHANNEL],
        //       fd[WRITE_CHANNEL], childPid);
        readFlag = 1;
        if (rc_find != SUCCESS) {
          rc_t = errorHandler(rc_find);
        } else {
          readFlag = SUCCESS;
        }
      } else {
        skipPath(candidate->path, relativePath, candidate->toSkip);
        pthread_mutex_lock(&(sharedResources->mutex));
        toSchedule = performInsert(relativePath, candidate->path,
                                   candidate->startingNode, IS_FILE, &rc_pi);
        if (rc_pi == SUCCESS) {
          rc_en = SUCCESS;
          rc_en = enqueue(sharedResources->fileToAssign, (void *)toSchedule);
          if (rc_en != SUCCESS) {
            rc_t = errorHandler(MALLOC_FAILURE);
          }
        } else {
          rc_t = errorHandler(rc_pi);
        }
        pthread_mutex_unlock(&(sharedResources->mutex));
        insertFlag = FAILURE;
      }
    }

    // Parte di lettura
    if (readFlag == SUCCESS) {
      bytesRead = read(fd[READ_CHANNEL], &charRead, 1);
      if (bytesRead > 0 || (kill(childPid, 0) == 0)) {
        if (bytesRead > 0) {
          candidate->path[counter + skipped] = charRead;
          if (skipped >= candidate->toSkip) {
            if (charRead == '\n' || charRead == '\0') {
              relativePath[counter] = '\0';
              candidate->path[counter + skipped] = '\0';
              if (counter > 0) {
                pthread_mutex_lock(&(sharedResources->mutex));
                rc_ia = insertAndSchedule(candidate->startingNode,
                                          sharedResources->fileToAssign,
                                          relativePath, candidate->path, sharedResources->requestedFiles);
                pthread_mutex_unlock(&(sharedResources->mutex));
                if (rc_ia != SUCCESS && rc_ia != ALREADY_INSERTED) {
                  rc_t = errorHandler(rc_ia);
                }
              }
              counter = 0;
              skipped = 0;
            } else {
              relativePath[counter] = charRead;
              counter++;
            }
          } else {
            skipped++;
          }
        }
      } else {
        closeDescriptor(fd[READ_CHANNEL]);
        readFlag = FAILURE;
        pthread_mutex_lock(&(sharedResources->mutex));
        destroyTreeNodeCandidate(candidate);
        pthread_mutex_unlock(&(sharedResources->mutex));
      }
    }
    usleep(500);
  }
  printf("Ho rct %d perche' sono morto FILEMANAGE \n", rc_t);
  free(relativePath);
  kill(getpid(), SIGKILL);
}

int errorHandler(int errorCode) {
  int rc_t = SUCCESS;
  switch (errorCode) {
  case READ_FAILURE:
    /* printInfo("reading from worker"); */
    rc_t = SUCCESS;
    break;
  case PIPE_FAILURE:
    printInfo("Something went wrong due to pipe bad behavior");
    rc_t = HARAKIRI;
    break;
  case CHARACTER_OVERFLOW:
    printInfo("Preventing characters overflow, the statistics could not be "
              "reliable");
    rc_t = SUCCESS;
    break;
  case MALLOC_FAILURE:
    printError("I could not allocate enough memory to run the program correctly");
    rc_t = HARAKIRI;
  case CWD_ACCESS_DENIED:
    printError("For some reasons that I do not know I could not access the current working directory");
    rc_t = HARAKIRI;
    break;
  case UNEXPECTED_LIST_ERROR:
    printError("Something gone wrong due to unespected list behavior");
    rc_t = HARAKIRI;
    break;
  case FORK_FAILURE:
    printError("The program couldn't execute a fork");
    rc_t = HARAKIRI;
    break;
  case NULL_POINTER:
    printInfo("Unexpected null pointer");
    rc_t = SUCCESS;
    break;
  case FIND_FAILURE:
    printError("Find function could not be called");
    rc_t = HARAKIRI;
    break;
  case DEAD_PROCESS:
    printInfo("One or more managers are dead, new one/s is/are spawned");
    rc_t = SUCCESS;
    break;
  case ASSIGNWORK_FILE_FAILURE:
    printInfo("No file to assign");
    rc_t = SUCCESS;
    break;
  case UNEXPECTED_PRIORITY_QUEUE_FAILURE:
    printError("Something went wrong due to unespected priority queue behavior");
    rc_t = HARAKIRI;
    break;
  case FILE_NOT_FOUND:
    //Niente da scrivere
    rc_t = SUCCESS;
    break;
  case ANALYZER_MANAGER_MISUNDERSTANDING:
    printInfo("There was a misunderstanding between Manager and Analyzer");
    rc_t = SUCCESS;
    break;
  case SSCANF_FAILURE:
    printInfo("Something went wrong due to unespected sscanf behavior");
    rc_t = SUCCESS;
    break;
  case NON_EXISTING_FILE:
    printError("The Infos of a node in the tree rapresenting the "
                           "File System are NULL");
    rc_t = HARAKIRI;
    break;
  case NON_EXISITING_FS_NODE:
    printError("A node in the tree rapresenting the File System "
                             "is NULL");
    rc_t = HARAKIRI;
    break;
  case SEND_FAILURE:
    printInfo("not able to send work to worker");
    rc_t = SUCCESS;
    break;
  case NEW_WORKER_FAILURE:
    printError("No memory for manager creation");
    rc_t = HARAKIRI;
    break;
  case MANAGER_CREATION_FAILURE:
    printError("Something went wrong in managers spawn");
    rc_t = HARAKIRI;
    break;
  case CAST_FAILURE:
    printInfo("Manager and Worker number must integers");
    rc_t = SUCCESS;
    break;
  case INVALID_SYNTAX_ERROR:
    printInfo("Invalid Argument passed from input, syntax must "
                "be:\n-Path\n-Number of Manager\n-Number of Worker\n");
    rc_t = SUCCESS;
    break;
  case FILE_TYPE_NOT_RECOGNIZED:
    printInfo("I didn't understand the file type of what you gave me");
    rc_t = SUCCESS;
  break;
  case SPRINTF_FAILURE:
    printError("Something went wrong due to bad ssprintf behavior");
    rc_t = SUCCESS;
  break;
  case READ_BASH_FAILURE:
    printError("Error reading from Bash");
    rc_t = HARAKIRI;
  break;
  case WRITE_BASH_FAILURE:
    printError("Error writing using Bash");
    rc_t = HARAKIRI;
  break;
  case BASH_FAILURE:
    printError("Bash can not be called");
    rc_t = HARAKIRI;
  break;
  default:
    printError("unknown error");
    rc_t = HARAKIRI;
    break;
  }

  return rc_t;
}

void *readString(int fd, char *dst) {
  int index = 0;
  int byteRead = -1;
  char charRead = 'a';

  while (byteRead != 0 && charRead != '\0') {
    byteRead = readChar(fd, &charRead);
    if (byteRead != 0) {
      dst[index++] = charRead;
    }
  }
  /* dst[index] = '\0'; */
}

int sendChildToReporter(TreeNode requested, int fd, char *toSend){
  Node currentChild = NULL;
  TreeNode child = NULL;
  FileInfo file = NULL;
  int rc_t = SUCCESS;
  int rc_wd = writeDescriptor(fd, "tree");
  fprintf(stderr, "Hai richiesto che ti sia inviato il signor: %s\n", ((FileInfo)requested->data)->name);
  if(requested->children != NULL && rc_wd >= 0){
    fprintf(stderr, "La cosa che hai chiesto ha size: %d\n", requested->children->size);
    int rc_ss = sprintf(toSend, "%d", requested->children->size);
    if(rc_ss < 0){
      // TODO... handle sprintf error
    } else {
      rc_wd = writeDescriptor(fd, toSend);
      if(rc_wd >= 0){
        currentChild = requested->children->head;
        while(currentChild != NULL){
          child = (TreeNode) currentChild->data;
          if(child != NULL){
            file = (FileInfo) child->data;
            if(file != NULL){
              fprintf(stderr, "Invio il figlio di nome %s che ha tipo %d\n", file->name, file->isDirectory);
              rc_wd = writeDescriptor(fd, file->name);
              if(rc_wd > 0){
                if(file->isDirectory == DIRECTORY){
                  rc_wd = writeDescriptor(fd, "d");
                } else {
                  rc_wd = writeDescriptor(fd, "f");
                }
                if(rc_wd < 0){
                  // TODO... not sure what to do
                }
              } else {
                // TODO... not sure what to do
              }
            } else {
              rc_t = NULL_POINTER;
            }
          } else {
            rc_t = NULL_POINTER;
          }
          currentChild = currentChild->next;
        }
      } else {
        // TODO... to handle???
        writeDescriptor(fd, "0");
      }
    }
  } else {
    // TODO... to handle???
    writeDescriptor(fd, "0");
  }
  return rc_t;
}

// TODO... use this to send the complete table and so change the parameters
int sendTableToReporter(int fd, long long unsigned *requestedFilesTable){
  int rc_t = SUCCESS;
  int j = 0;
  char *number = malloc(PATH_MAX * sizeof(char));
  fprintf(stderr,"Entro e poi mi spacco\n");
  if(requestedFilesTable != NULL){
    for (j = 0; j < NCHAR_TABLE; j++) {
      int rc_sp;
      rc_sp = sprintf(number, "%llu", requestedFilesTable[j]);
      if (rc_sp == -1)
        rc_t = CAST_FAILURE;
      else {
        fprintf(stderr, "Ho scritto cose brutte indice %d schifo da scrivere %s\n", j, number);
        int rc_wr = writeDescriptor(fd, number);
        if (rc_wr == -1)
          rc_t = SUMMARY_FAILURE;
      }
      //sleep(1);
    }
  } else {
    rc_t = NULL_POINTER;
  }
  fprintf(stderr, "Ho un codice di errore strambo che vale %d\n", rc_t);
  free(number);
  return rc_t;
}

void *writeOnFIFOLoop(void *ptr){
  sharedResourcesAnalyzer_t *sharedResources = (sharedResourcesAnalyzer_t *)ptr;
  TreeNode requested = NULL;
  char *toSend = (char *) malloc(sizeof(char) * PATH_MAX);
  int rc_t = checkAllocationError(toSend);
  int msg = SUCCESS;
  int rc_wd = SUCCESS;
  char *writeFifo = "/tmp/analyzerToReporter";
  int rc_fi = mkfifo(writeFifo, 0666);
  // TODO... check for all the erorrs
  while(rc_t == SUCCESS){
    pthread_mutex_lock(&(sharedResources->mutex));
    if(sharedResources->toRetrive != NULL){
      pthread_mutex_unlock(&(sharedResources->mutex));
      int fd = open(writeFifo, O_WRONLY);
      pthread_mutex_lock(&(sharedResources->mutex));
      if(fd > 0){
        requested = performInsert(sharedResources->toRetrive, NULL, getRoot(sharedResources->fs), DIRECTORY, &msg);
        // TODO... handle with error handler
        if(msg == SUCCESS || msg == ALREADY_INSERTED && requested != NULL){
          sharedResources->currentDirectory = requested;
          // TODO... handle strcpy with wrapper function
          strcpy(sharedResources->cwd, sharedResources->toRetrive);
          rc_t = sendChildToReporter(requested, fd, toSend);
        } else {
          rc_wd = writeDescriptor(fd, "0");
        }
      }
      sharedResources->toRetrive = NULL;
      close(fd);
      fprintf(stderr, "chiudo la fifo\n");
      pthread_mutex_unlock(&(sharedResources->mutex));
    } else {
      pthread_mutex_unlock(&(sharedResources->mutex));
    }

    pthread_mutex_lock(&(sharedResources->mutex));
    if(sharedResources->sendChanges == SUCCESS){
      pthread_mutex_unlock(&(sharedResources->mutex));
      fprintf(stderr, "STO PROVANDO A MANDARE DEGLI AGGIORNAMENTI\n");
      int fd = open(writeFifo, O_WRONLY);
      fprintf(stderr, "Ho 43passato la creazione/apertura della fifo\n");
      pthread_mutex_lock(&(sharedResources->mutex));
      if(fd > 0){
        rc_wd = writeDescriptor(fd, "tabl");
        if(rc_wd > 0){
          fprintf(stderr, "Sto per inviare le tabelle al reporter\n");
          int rc_st = sendTableToReporter(fd, sharedResources->requestedFilesTable);
          fprintf(stderr, "Ho scritto la tabella per il sig reporter\n");
          if(rc_st != SUCCESS){
            rc_t = errorHandler(rc_st);
          }
        }
      }
      sharedResources->sendChanges = -1;
      close(fd);
      pthread_mutex_unlock(&(sharedResources->mutex));
    } else {
      pthread_mutex_unlock(&(sharedResources->mutex));
    }

  }
  fprintf(stderr, "SONO MORTO PERCHE' HO RC_T = %d\n", rc_t);
  kill(getpid(), SIGKILL);
}

int changeWorkerAmount(PriorityQueue managers, const int amount){
  int rc_t = SUCCESS;
  int rc_ss = SUCCESS;
  int rc_ss2 = SUCCESS;
  int rc_pu = SUCCESS;
  int rc_sw = SUCCESS;
  int *fd;
  int managerSize = 0;
  char *stopMsg = malloc(sizeof(char) * CONTROL_WORD_LEN);
  int rc_al = checkAllocationError(stopMsg);
  char *nWorker = malloc(sizeof(char) * INT_MAX_LEN);
  int rc_al2 = checkAllocationError(nWorker);
  PriorityQueue tmpManagers = newPriorityQueue();
  if(tmpManagers == NULL || rc_al < SUCCESS || rc_al2 < SUCCESS){
    rc_t = MALLOC_FAILURE;
  } else {
    rc_ss = sprintf(stopMsg, "%s", CONTROL_STOP);
    rc_ss2 = sprintf(nWorker, "%d", amount);
    if (rc_ss < 0 || rc_ss2 < 0) {
      rc_t = SPRINTF_FAILURE;
    }
  }
  if(managers != NULL){
    managerSize = managers->len;
  } else {
    rc_t = NULL_POINTER;
  }
  while (rc_t == SUCCESS && managerSize != 0) {
    Manager m = popPriorityQueue(managers);
    if(m != NULL){
      fd = m->pipe;
      if(fd != NULL && fd > 0){
        int rc_wr = writeDescriptor(fd[WRITE_CHANNEL], stopMsg);
        int rc_wr2 = writeDescriptor(fd[WRITE_CHANNEL], nWorker);
        if (rc_wr < SUCCESS || rc_wr2 < SUCCESS) {
          rc_t = SEND_FAILURE;
        } 
      }else{
        //TODO... handle the error
      }
    } else {
      rc_t = UNEXPECTED_PRIORITY_QUEUE_FAILURE;
    }
    rc_pu = pushPriorityQueue(tmpManagers, m->filesInExecution->size, (void *)m);
    if(rc_t == SUCCESS && rc_pu < SUCCESS){
      rc_t = errorHandler(MALLOC_FAILURE);
    }
    managerSize--;
  }
  rc_sw = swapPriorityQueue(managers, tmpManagers);
  destroyPriorityQueue(tmpManagers, free);
  if(rc_sw != SUCCESS){
    rc_t = errorHandler(UNEXPECTED_PRIORITY_QUEUE_FAILURE);
  }
  free(stopMsg);
  free(nWorker);
  return rc_t;
}

int resetRequestedFile(List requestedFiles, long long unsigned *requestedFilesTable){
  int rc_t = SUCCESS;
  int rc_po = SUCCESS;
  int i = 0;
  TreeNode requested = NULL;
  FileInfo file = NULL;
  int requestedSize = 0;
  if(requestedFiles != NULL){
    requestedSize = requestedFiles->size;
    while(requestedSize > 0){
      requested = (TreeNode) front(requestedFiles);
      rc_po = pop(requestedFiles);
      if(requested != NULL && rc_po == SUCCESS){
        file = (FileInfo) requested->data;
        if(file != NULL){
          file->isRequested = -1;
        } else {
          rc_t = NULL_POINTER;
        }
      } else {
        rc_t = UNEXPECTED_LIST_ERROR;
      }
      requestedSize--;
    }
  } else {
    rc_t = NULL_POINTER;
  }

  if(requestedFilesTable != NULL){
    for(i = 0; i < NCHAR_TABLE; i++){
      requestedFilesTable[i] = 0;
    }
  } else {
    rc_t = NULL_POINTER;
  }
  
  return rc_t;
}

int sumTables(long long unsigned *dst, long long unsigned *src, const int dim){
  int rc_t = SUCCESS;
  int i = 0;
  if(dst != NULL && src != NULL){
    for(i = 0; i < dim; i++){
      if(ULLONG_MAX - dst[i] >= src[i]){
        dst[i] += src[i];
      }
    }
  } else {
    rc_t = NULL_POINTER;
  }
  return rc_t;
}

void *readFromFIFOLoop(void *ptr){
  sharedResourcesAnalyzer_t *sharedResources = (sharedResourcesAnalyzer_t *)ptr;
  char *readFifo = "/tmp/reporterToAnalyzer";
  int rc_fi = mkfifo(readFifo, 0666);
  // TODO... check allocation of the list
  List dire = newList();
  TreeNode startingNode = NULL;
  TreeNodeCandidate toAnalyze = NULL;
  TreeNode requested = NULL;
  FileInfo requestedFile = NULL;
  int rc_t = SUCCESS;
  int rc_en = SUCCESS;
  int rc_st = SUCCESS;
  int rc_rd = SUCCESS;
  int rc_cma = SUCCESS;
  int rc_ct = SUCCESS;
  int rc_an = SUCCESS;
  int rc_al = SUCCESS;
  int rc_al2 = SUCCESS;
  int rc_al3 = SUCCESS;
  int rc_al4 = SUCCESS;
  int rc_al5 = SUCCESS;
  char *tmpCwd = malloc(sizeof(char) * PATH_MAX);
  rc_al4 = checkAllocationError(tmpCwd);
  int newNManager = 0;
  int newNWorker = 0;
  int type;
  int toSkip;

  if (rc_al4 != SUCCESS) {
    rc_t = MALLOC_FAILURE;
  } else {
    pthread_mutex_lock(&(sharedResources->mutex));
    // TODO... check strcpy errors with the special function created just for this purpose
    strcpy(tmpCwd, sharedResources->cwd);
    pthread_mutex_unlock(&(sharedResources->mutex));
  }
  // TODO... add checks
  while (rc_t == SUCCESS) {
    int fd = open(readFifo, O_RDONLY);
    // printf("fd: %d\n", fd);
    while (1) {
      char *dst = malloc(PATH_MAX * sizeof(char));
      dst[0] = '\0';
      readString(fd, dst);
      /* printf("parola di controllo: %s\n", dst); */
      if (strcmp(dst, "dire") == 0) {
        char *newPath = front(dire);
        /* printf("size %d\n", dire->size); */
        printf("path: %s\n", newPath);
        pop(dire);
        char *nManager = front(dire);
        /* printf("size %d\n", dire->size); */
        printf("manager: %s\n", nManager);
        pop(dire);
        char *nWorker = front(dire);
        /* printf("size %d\n", dire->size); */
        printf("worker: %s\n", nWorker);
        pop(dire);
        rc_ct = SUCCESS;
        int rc_sc = sscanf(nManager, "%d", &newNManager);
        int rc_sc2 = sscanf(nWorker, "%d", &newNWorker);
        if (rc_sc == 0 || (newNManager == 9 && strcmp(nManager, "9") != 0) ||
            nManager[0] == 0) {
          rc_ct = CAST_FAILURE;
        }
        if (rc_sc2 == 0 || (newNWorker == 9 && strcmp(nWorker, "9") != 0) ||
            nWorker[0] == 0) {
          rc_ct = CAST_FAILURE;
        }

        if (rc_rd == SUCCESS && rc_ct != CAST_FAILURE) {
          pthread_mutex_lock(&(sharedResources->mutex));
          *(sharedResources->nWorker) = newNWorker;
          // TODO... same as above
          strcpy(sharedResources->path, newPath);
          if (*(sharedResources->nManager) != newNManager) {
            rc_cma = changeManagersAmount(sharedResources->managers,
                                *(sharedResources->nManager), newNManager,
                                sharedResources->fileToAssign);
            if(rc_cma != SUCCESS){
              rc_t = errorHandler(rc_cma);
            }
            *(sharedResources->nManager) = newNManager;
          }
          pthread_mutex_unlock(&(sharedResources->mutex));
        } else {
          rc_t = errorHandler(INVALID_SYNTAX_ERROR);
        }
        
        if (rc_ct == SUCCESS) {
          // TODO... copy the if else in the other thread
          if(strcmp(newPath, "//") == 0){
            pthread_mutex_lock(&(sharedResources->mutex));
            changeWorkerAmount(sharedResources->managers, *(sharedResources->nWorker));
            pthread_mutex_unlock(&(sharedResources->mutex));
          } else {
            type = precomputeAnalyzerInput(tmpCwd, newPath, &rc_an);
            if (rc_an != SUCCESS) {
              if (rc_an != EMPTY_PATH) {
                rc_t = errorHandler(rc_an);
              }
            } else {
              if (type == DIRECTORY) {
                addForwardSlash(newPath);
              }
              if (type == DIRECTORY || type == IS_FILE) {
                toSkip = 0;
                startingNode = NULL;
                pthread_mutex_lock(&(sharedResources->mutex));
                startingNode = precomputeStartingDirectory(
                    sharedResources->fs, sharedResources->currentDirectory, newPath,
                    &toSkip);
                toAnalyze = newTreeNodeCandidate(startingNode, type, newPath, toSkip);
                if (toAnalyze != NULL) {
                  // TODO... check push return code
                  push(sharedResources->candidateNode, toAnalyze);
                } else {
                  rc_t = errorHandler(MALLOC_FAILURE);
                }
                pthread_mutex_unlock(&(sharedResources->mutex));
              } else {
                if (type == NOT_EXISTING) {
                  char *msgInfo = (char *)malloc(sizeof(char) * (PATH_MAX + 300));
                  rc_al5 = checkAllocationError(msgInfo);
                  if (rc_al5 < 0) {
                    rc_t = errorHandler(MALLOC_FAILURE);
                  } else {
                    sprintf(msgInfo, "The File %s doesn't exist", newPath);
                    printInfo(msgInfo);
                    free(msgInfo);
                  }
                } else {
                  rc_t = errorHandler(FILE_TYPE_NOT_RECOGNIZED);
                }
              }
            }
          }
        }
        break;
      } else if (strcmp(dst, "requ") == 0) {
        pthread_mutex_lock(&(sharedResources->mutex));
        rc_t = resetRequestedFile(sharedResources->requestedFiles, sharedResources->requestedFilesTable);
        while (dire->size != 0) {
          char *requestedPath = front(dire);
          if(requestedPath != NULL){
            requested = performInsert(requestedPath, NULL, getRoot(sharedResources->fs), DIRECTORY, &rc_t);
            // TODO... handle with error handler
            if(rc_t == SUCCESS || rc_t == ALREADY_INSERTED && requested != NULL){
              requestedFile = (FileInfo) requested->data;
              if(requestedFile != NULL){
                requestedFile->isRequested = SUCCESS;
                rc_st = sumTables(sharedResources->requestedFilesTable, requestedFile->fileTable, NCHAR_TABLE);
                if(rc_st != SUCCESS){
                  rc_t = errorHandler(rc_st);
                }
              } else {
                rc_t = NULL_POINTER;
              }
              rc_en = enqueue(sharedResources->requestedFiles, (void *) requested);
              if(rc_en != SUCCESS){
                rc_t = rc_en;
              }
            }
          } else {
            rc_t = NULL_POINTER;
          }
          printf("file: %s\n", requestedPath);
          pop(dire);
        }
        pthread_mutex_unlock(&(sharedResources->mutex));
        break;
      } else if (strcmp(dst, "tree") == 0) {
        char *toRetrive = front(dire);
        printf("voglio i figli di : %s\n", toRetrive);
        if(toRetrive != NULL){
          pthread_mutex_lock(&(sharedResources->mutex));
          sharedResources->toRetrive = toRetrive;
          pthread_mutex_unlock(&(sharedResources->mutex));
          pop(dire);
        } else {
          // TODO... unexpected list error probably
        }
        break;
      } else if (strcmp(dst, "") != 0) {
        enqueue(dire, dst);
        /* printf("ho encodato %s, newSize %d\n", dst, dire->size); */
      } else {
        //printf("ho fatto la free\n");
        free(dst);
      }
      usleep(1);
    }
    close(fd);
    printf("chiudo\n");
  }
}

int main() {
  signal(SIGCHLD, SIG_IGN);
  //signal(SIGINT, sighandle_print);
  sharedResourcesAnalyzer_t sharedResources;
  Tree fs = NULL;
  TreeNode currentDirectory = NULL;
  FileInfo root = NULL;
  PriorityQueue managers = newPriorityQueue();
  List fileToAssign = newList();
  List candidateNode = newList();
  List requestedFiles = newList();
  char *cwd = (char *)malloc(sizeof(char) * PATH_MAX);
  int rc_al = checkAllocationError(cwd);
  char *path = (char *)malloc(sizeof(char) * PATH_MAX);
  int rc_al2 = checkAllocationError(path);
  long long unsigned *requestedFilesTable = (long long unsigned *) calloc(NCHAR_TABLE, sizeof(long long unsigned));
  int rc_al3 = checkAllocationError(requestedFilesTable);
  int defaultManagers = 3;
  int defaultWorkers = 4;
  int rc_cwd = SUCCESS;
  int rc_ir = SUCCESS;
  int rc_it = SUCCESS;
  int rc_pi = SUCCESS;
  int rc_t = SUCCESS;
  int rc_am = SUCCESS;
  int rc_en = SUCCESS;
  rc_am = addManagers(managers, defaultManagers);
  rc_cwd = getCwd(cwd);
  if(rc_cwd != SUCCESS){
    rc_t = errorHandler(rc_cwd);
  }
  if(rc_t == SUCCESS && rc_am != SUCCESS){
    rc_t = errorHandler(rc_am);
  }
  if(rc_t == SUCCESS && rc_al != SUCCESS){
    rc_t = errorHandler(rc_al);
  }
  if(rc_t == SUCCESS && rc_al2 != SUCCESS){
    rc_t = errorHandler(rc_al2);
  }
  if(rc_t == SUCCESS && rc_al3 != SUCCESS){
    rc_t = errorHandler(rc_al3);
  }
  if(rc_t == SUCCESS && (managers == NULL || candidateNode == NULL || fileToAssign == NULL || requestedFiles == NULL)){
    rc_t = MALLOC_FAILURE;
  }
  if (rc_t == SUCCESS) {
    root = initFileInfoRoot(&rc_ir);
    if (rc_ir == SUCCESS) {
      // Inizializzazione dell'albero
      fs = initializeAnalyzerTree(root, &rc_it, destroyFileInfo);
      if (rc_it == SUCCESS) { 
        rc_en = enqueue(requestedFiles, getRoot(fs));
        if(rc_en == SUCCESS){
          currentDirectory =
            performInsert(cwd, NULL, getRoot(fs), DIRECTORY, &rc_pi);
          if (rc_pi != SUCCESS) {
            rc_t = errorHandler(rc_pi);
          }
        }else{
          rc_t = errorHandler(rc_en);
        }
      } else {
        rc_t = errorHandler(rc_it);
      }
    } else {
      rc_t = errorHandler(rc_ir);
    }
    sharedResources.cwd = cwd;
    sharedResources.requestedFiles = requestedFiles;
    sharedResources.fileToAssign = fileToAssign;
    sharedResources.currentDirectory = currentDirectory;
    sharedResources.fs = fs;
    sharedResources.managers = managers;
    sharedResources.nManager = &defaultManagers;
    sharedResources.nWorker = &defaultWorkers;
    sharedResources.path = path;
    sharedResources.candidateNode = candidateNode;
    sharedResources.toRetrive = NULL;
    sharedResources.requestedFilesTable = requestedFilesTable;
    sharedResources.sendChanges = -1;
    pthread_t reads;
    pthread_mutex_init(&sharedResources.mutex, NULL);
    pthread_create(&reads, NULL, readDirectivesLoop, (void *)&sharedResources);
    pthread_t fileManage;
    pthread_create(&fileManage, NULL, fileManageLoop, (void *)&sharedResources);
    pthread_t send;
    pthread_create(&send, NULL, sendFileLoop, (void *)&sharedResources);
    pthread_t readFIFO;
    pthread_create(&fileManage, NULL, readFromFIFOLoop, (void *)&sharedResources);
    pthread_t writeFIFO;
    pthread_create(&fileManage, NULL, writeOnFIFOLoop, (void *)&sharedResources);
    pthread_join(reads, NULL);
    pthread_join(fileManage, NULL);
    pthread_join(send, NULL);
    pthread_join(readFIFO, NULL);
    pthread_join(writeFIFO, NULL);
  } else {
    rc_t = errorHandler(rc_t);
  }
  destroyTree(fs);
  destroyPriorityQueue(managers, destroyManager);
  return rc_t;
}
/*


                /
               /
  |||||| °°°  |
              |
              |
  ||||||      |
               \
                \


*/
