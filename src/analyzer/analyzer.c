#include "./analyzer.h"
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
#include "../config/config.h"

int spawnFindProcess(char *compactedPath, int *fd, int *childPid);

FileInfo newFileInfo(char *name, int isDirectory, char *path, int *msg);

int allocatePath(FileInfo fileinfo, char *string);

void destroyFileInfo(void *toDestroy);

int checkStrcpy(char *dst, char *src);

Node handleMatch(TreeNode toExamine, FileInfo dataToExamine, char *path,
                 int *found, int *match, int *counter, int *resetCounter,
                 int *tmpCounter);

int checkIfMatch(FileInfo dataToExamine, char *path, int *counter,
                 int *resetCounter, int *tmpCounter);

Node getNodeWhereToInsert(TreeNode toExamine, FileInfo dataToExamine,
                          Node actualNode, char *path, int *found, int *match,
                          int *counter, int *resetCounter, int *tmpCounter);

TreeNode createNewTreeElement(FileInfo dataToInsert, TreeNode toInsert,
                              TreeNode whereToInsert, int *tmpCounter,
                              char *pathName, int isDirectory,
                              char *completePath, int *msg);

TreeNode createNewTwig(TreeNode whereToInsert, int *counter, char *path,
                       char *pathName, int isDirectory, char *completePath,
                       int *msg);

TreeNode performInsert(char *path, char *completePath, TreeNode startingPoint,
                       int isDirectory, int *msg);

int scheduleFile(TreeNode toSchedule, Manager manager);

int insertAndSchedule(TreeNode startingNode, List filesToAssign,
                      char *relativePath, char *completePath);

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

int readBashLines(int *fd, char *dst, int childPid);

int writeBashProcess(int *fd, char *argv[]);

int callBashProcess(char *dst, char *argv[]);

int checkFileType(char *name);

int compactPath(char *string, char *cwd, int isAbsolute);

TreeNode goUp(Tree fs, TreeNode currentDirectory, char *path, int *toSkip);

TreeNode precomputeStartingDirectory(Tree fs, TreeNode currentDirectory,
                                     char *path, int *toSkip);

void addForwardSlash(char *path);

int checkAnalyzeAndAssign(Tree fs, TreeNode currentDirectory, char *path,
                          int *toSkip, PriorityQueue managers,
                          const int *nManager, const int *nWorker,
                          List filesToAssign);

int readDirectives(char *path, char *nManager, char *nWorker);

int doWhatAnalyzerDoes(char *cwd, Tree fs, TreeNode currentDirectory,
                       PriorityQueue managers, int *nManager, int *nWorker,
                       List fileToAssign, char *path);

int managerInitPipe(const int toParent[], const int toChild[]);

int parentInitExecPipe(const int toParent[], const int toChild[]);

Manager newManager();

int addManagers(PriorityQueue managers, int amount);

int endManager(Manager m, List fileToAssign);

void destroyManager(void *data);

int removeManagers(PriorityQueue managers, int amount, List fileToAssign);

int changeManagersAmount(PriorityQueue managers, const int currentManagers,
                         const int newManagers, List fileToAssign);

int sendFile(Manager manager, TreeNode file, List filesToAssign,
             int currentWorker);

int rescheduleFiles(List toDestroy, List toReschedule);

int manageFileToSend(PriorityQueue managers, int currentWorker,
                     List filesToAssign);

FileInfo initFileInfoRoot(int *msg);

Tree initializeAnalyzerTree(FileInfo data, int *msg, void destroyer(void *));

int getCwd(char *dst);

void *fileManageLoop(void *ptr);

void *sendFileLoop(void *ptr);

void *readDirectivesLoop(void *ptr);

void skipPath(char *path, char *relativePath, int toSkip);

void toStringTable(long long unsigned * table);

List finished = NULL;

void toStringTable(long long unsigned int * table){
  int i;
  for(i = 0; i < NCHAR_TABLE; i++){
    printf("%d - %llu\n", i+1, table[i]);
  }
}

void destroyTreeNodeCandidate(void *data) {
  TreeNodeCandidate candidate = (TreeNodeCandidate)data;
  free(candidate->path);
  free(candidate);
}

void printCandidateNode(void *data) {
  TreeNodeCandidate candidate = (TreeNodeCandidate)data;
  printf("Starting node: %p, path: %s, tof: %d, toSkipper: %d\n",
         candidate->startingNode, candidate->path, candidate->type,
         candidate->toSkip);
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

/**
 * Costructor of the file info structure
 */
FileInfo newFileInfo(char *name, int isDirectory, char *path, int *msg) {
  FileInfo toRtn = (FileInfo)malloc(sizeof(struct FileInfo));
  *msg = checkAllocationError(toRtn);
  if (*msg == SUCCESS) {
    toRtn->name = name;
    toRtn->isDirectory = isDirectory;
    toRtn->fileTable =
        (unsigned long long *)calloc(NCHAR_TABLE, sizeof(unsigned long long));
    *msg = checkAllocationError(toRtn->fileTable);
    if (*msg == SUCCESS) {
      *msg = allocatePath(toRtn, path);
      if (*msg != SUCCESS) {
        // printf("free file table\n");
        free(toRtn->fileTable);
        // printf("free toRtn\n");
        free(toRtn);
        // printf("dopo free toRtn\n");
      }
    } else {
      // printf("free toRtn 2\n");
      free(toRtn);
      // printf("dopo free toRtn 2\n");
    }
  }
  return toRtn;
}

/**
 * Function that allocate path and check if it return a failure
 */
int allocatePath(FileInfo fileinfo, char *string) {
  int rc_a = SUCCESS;
  if (string != NULL) {
    fileinfo->path = (char *)malloc(sizeof(char) * PATH_MAX);
    rc_a = checkAllocationError(fileinfo->name);
    if (rc_a == SUCCESS) {
      rc_a = checkStrcpy(fileinfo->path, string);
    } else {
      printf("Muoio sempre in allocate path 111\n");
    }
  } else {
    fileinfo->path = NULL;
  }
  return rc_a;
}

/**
 * Destructor of the file info structure
 */
void destroyFileInfo(void *toDestroy) {
  FileInfo data = (FileInfo)toDestroy;
  printf("currently destroying %s\n", (char *)(data->name));
  // printf("free destroyFileInfo\n");
  free(data->name);
  free(data->path);
  free(data->fileTable);
  free(toDestroy);
  // printf("dopo destroyFileInfo\n");
}

/**
 * Function that check for strcpy
 */
int checkStrcpy(char *dst, char *src) {
  int rc_cp = SUCCESS;
  if (strcpy(dst, src) == NULL) {
    rc_cp = FAILURE;
  }
  return rc_cp;
}

/**
 *  File info toString
 */
void pasta(void *data) {
  printf("File : %s name %s isFolder %d\n",
         ((FileInfo)((TreeNode)data)->data)->path,
         ((FileInfo)((TreeNode)data)->data)->name,
         ((FileInfo)((TreeNode)data)->data)->isDirectory);
}

void pasta2(void *data) {
  printf("File : %s name %s isFolder %d\n", ((FileInfo)data)->path,
         ((FileInfo)data)->name, ((FileInfo)data)->isDirectory);
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
                      char *relativePath, char *completePath) {
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
  if (rc_t == SUCCESS) {
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
 * Perform insert
 */
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
  // printf("Ricevo -> path: %s, completePath: %s, isDirectory %d, msg %d\n",
  // path, completePath, isDirectory, *msg); printf("size: %d\n",
  // startingPoint->children->size);
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
    // printf("found %d\n", found);
    if (found == 0) { // The new Element wasn't a children of mine then i need
                      // to add it to my children
      if (toExamine ==
          NULL) { // I need to attach the children to the Starting Point
        whereToInsert = startingPoint;
      } else { // The new node is my brother so I need to attach the children to
               // my father
        whereToInsert = toExamine->parent;
      }
    } else if (found == 1) { // The element it's my first child (Probably never
                             // going to happen buuuuuut... YNK)
      whereToInsert = toExamine;
    }
    if (found != -1) {
      // printf("sono il padre del nodo che stai analizzando e sono %s\n",
      // ((FileInfo)whereToInsert->data)->name); printf("size: %d\n",
      // whereToInsert->children->size); EFFICENCY OVER THE MEMORY OVER NINE
      // THOUSAND
      char *pathName = (char *)malloc(sizeof(char) * PATH_MAX);
      // tmpCounter = 0;
      rc_ca = checkAllocationError(pathName);
      if (rc_ca == SUCCESS) {
        toRtn = createNewTwig(whereToInsert, &counter, path, pathName,
                              isDirectory, completePath, &rc_ca);
      }
      *msg = rc_ca;
    } else {
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
      // printf("counter %d\n", counter);
      // printf("controllo %s, tmpCounter = %d\n", dataToExamine->name,
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
      // printf("counter = %d, tmpCounter = %d\n", *counter, *tmpCounter);
      // printf("\tmatch carattere\n");
    } else {
      // printf("\tnon faccio match per colpa di %c che dovrebbe essere %c\n",
      // dataToExamine->name[*tmpCounter], path[*counter]);
      *counter = *resetCounter;
      match = -1;
    }
  }
  // printf("counter = %d, resetCounter = %d, match = %d\n", *counter,
  // *resetCounter, match);
  return match;
}

Node handleMatch(TreeNode toExamine, FileInfo dataToExamine, char *path,
                 int *found, int *match, int *counter, int *resetCounter,
                 int *tmpCounter) {
  Node actualNode = NULL;
  if (*match == 0) {
    if (dataToExamine->name[*tmpCounter] ==
        '\0') { // Two names matches perfectly
      // printf("MATCH\n");
      if (path[*counter] == '\0') { // It's already a folder or a file in the
                                    // actual tree no need to insert again
        *found = -1;
        // printf("File Already Inserted\n");
      } else {
        // printf("%s è l'uomo che fa per me\n", dataToExamine->name);
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
      // printf("I'm a false positive\n");
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
  int tmpCounter = 0;
  while (path[*counter] != '\0' && *msg == SUCCESS) {
    // printf("giro nel ciclo\n");
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
  // printf("file: %s\n", pathName);
  dataToInsert = newFileInfo(pathName, isDirectory, completePath, &rc_nc);
  toInsert = newTreeNode(whereToInsert, (void *)dataToInsert, &rc_tc);
  if (rc_nc == SUCCESS && rc_tc == SUCCESS) {
    linkChild(whereToInsert, toInsert);
  } else {
    // printf("free createNewTreeElement\n");
    free(dataToInsert);
    free(toInsert);
    // printf("dopo free createNewTreeElement\n");
    *msg = MALLOC_FAILURE;
  }
  return toInsert;
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

/**
 * Function that read a line from bash call into the pipe opened
 */
int readBashLines(int *fd, char *dst, int childPid) {
  int rc_t = SUCCESS;
  int rc_cds_1;
  int rc_cds_2;
  int bytesRead = 1;
  char charRead;
  int counter = 0;
  rc_cds_1 = closeDescriptor(fd[WRITE_CHANNEL]);
  // Ho messo il dowhile
  while (bytesRead > 0 || (kill(childPid, 0) == 0)) {
    // printf("carattere: %d\n", charRead);
    bytesRead = read(fd[READ_CHANNEL], &charRead, 1);
    if (bytesRead > 0) {
      if (charRead != '\n') {
        dst[counter] = charRead;
        counter++;
      }
    }
  }
  dst[counter] = '\0';
  // TODO... Deal with pipe
  rc_cds_2 = closeDescriptor(fd[READ_CHANNEL]);
  // Missing error handling
  if (rc_cds_1 != SUCCESS) {
    printError("Allllllllllora, non funziona la close descriptor in fd write "
               "in read bash line");
  } else if (rc_cds_2 != SUCCESS) {
    printError("Allllllllllora, non funziona la close descriptor in fd read in "
               "read bash line");
  }
  return rc_t;
}

/**
 * Function that call bash process and write on the pipe
 * It uses dup2 to override it's stdout
 */
int writeBashProcess(int *fd, char *argv[]) {
  // TODO... Fix the pipe in order to be not blocking
  int rc_t;
  int rc_cds_1 = closeDescriptor(fd[READ_CHANNEL]);
  int rc_cdp = createDup(fd[WRITE_CHANNEL], 1);
  int rc_cds_2 = closeDescriptor(fd[WRITE_CHANNEL]);
  int rc_ex = execvp(argv[0], argv);
  if (rc_ex < 0) {
    char *msgErr = (char *)malloc(sizeof(char) * 1000);
    int rc_ca = checkAllocationError(msgErr);
    if (rc_ca < 0) {
      // TODO... HARAKIRI PROBABLY
      printError("I can't allocate memory");
    } else {
      sprintf(msgErr, "Error while doing exec %s from bashCall\n", argv[0]);
      perror(msgErr);
      free(msgErr);
    }
    printf("FAILA LA BASHHHHHH\n");
    kill(getpid(), SIGKILL);
  }
  return rc_t;
}

/**
 * Function that call a bash process and pass to it the argv parameter
 * In case of success returns in dst what the bash prints
 */
int callBashProcess(char *dst, char *argv[]) {
  int rc_t = SUCCESS;
  int rc_rb;
  int rc_wb;
  int fd[2];
  int rc_fd = createUnidirPipe(fd);
  fcntl(fd[READ_CHANNEL], F_SETFL, O_NONBLOCK);
  if (rc_fd == SUCCESS) {
    int f = fork();
    if (f > 0) {
      rc_rb = readBashLines(fd, dst, f);
    } else if (f == 0) {
      rc_wb = writeBashProcess(fd, argv);
    } else {
      printError("The program couldn't execute a fork");
      rc_t = FORK_FAILURE;
    }
    if (rc_t != SUCCESS) {
      if (rc_rb != SUCCESS) {
        printError("Error in call bash process -> read");
      } else if (rc_wb != SUCCESS) {
        printError("Error in call bash process -> write");
      }
    }
  } else {
    printError("Pipe creation in call bash process gone wrong");
  }

  return rc_t;
}

/**
 * Function that check the file type by calling a bash process
 */
int checkFileType(char *name) {
  int rc_t = SUCCESS;
  int rc_sp = SUCCESS;
  int rc_ss = SUCCESS;
  char *command = (char *)malloc(sizeof(char) * (PATH_MAX * 2 + 150));
  rc_t = checkAllocationError(command);
  if (rc_t < 0) {
    // TODO... HARAKIRI PROBABLY
    printError("I can't allocate memory");
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
          fprintf(stderr,
                  "ERROR: Error in checkFileType due to scanf, is it really "
                  "able to fail? type: %s rc_t %d rc_ss %d\n",
                  type, rc_t, rc_ss);
          // Scegliere cosa fare, se rimediabile o meno, blocchiamo tutto o non
          // permettiamo di inserire il dato all'interno del comando?
        }
      }
    } else {
      // Stessa cosa di sopra
      printError(
          "Error in checkFileType due to sprintf, is it really able to fail?");
    }
    // printf("free command\n");
    free(command);
    // printf("dopo free command");
  }
  // Handle error
  return rc_t;
}

/**
 * Function that compact the path given from input by calling a bash function
 */
int compactPath(char *string, char *cwd, int isAbsolute) {
  int rc_t = SUCCESS;
  if (isAbsolute == ABSOLUTE) {
    char *argv[4] = {"realpath", "-m", string, NULL};
    rc_t = callBashProcess(string, argv);
  } else {
    char *argv[7] = {"realpath", "-m", "--relative-to", cwd, string, NULL};
    rc_t = callBashProcess(string, argv);
  }
  return rc_t;
}

/**
 * Function in precompute path that goes up
 */
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

/**
 * Function that precomputes the starting directory in wich files will be addes
 */
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
          printInfo("You created a monster of a folder but I still handled the "
                    "Error");
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
  // printf("devo skippare %d carattere/i e la working directory e' %s\n",
  // *toSkip, ((FileInfo)startingNode->data)->name);
  return startingNode;
}

/**
 * Add Forward Slash if it is not empty
 */
void addForwardSlash(char *path) {
  int stringLength = strlen(path);
  if (path[stringLength - 1] != '/') {
    path[stringLength] = '/';
    path[stringLength + 1] = '\0';
  }
}

int readDirectives(char *path, char *nManager, char *nWorker) {
  char readBuffer[2] = "a";
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

  printf("Path: %s\n", path);
  printf("Manager: %s\n", nManager);
  printf("Worker: %s\n", nWorker);
  return rc_t;
}

void *readDirectivesLoop(void *ptr) {
  sharedResourcesAnalyzer_t *sharedResources = (sharedResourcesAnalyzer_t *)ptr;
  TreeNode startingNode;
  TreeNodeCandidate toAnalyze;
  int rc_t = SUCCESS;
  int rc_rd = SUCCESS;
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
    strcpy(tmpCwd, sharedResources->cwd);
    pthread_mutex_unlock(&(sharedResources->mutex));
  }

  // TODO... Check allocation
  while (rc_t == SUCCESS) {
    rc_ct = SUCCESS;
    rc_rd = readDirectives(newPath, nManager, nWorker);
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

    if (rc_ct != CAST_FAILURE) {
      pthread_mutex_lock(&(sharedResources->mutex));
      *(sharedResources->nWorker) = newNWorker;
      strcpy(sharedResources->path, newPath);
      if (*(sharedResources->nManager) != newNManager) {
        changeManagersAmount(sharedResources->managers,
                             *(sharedResources->nManager), newNManager,
                             sharedResources->fileToAssign);
        *(sharedResources->nManager) = newNManager;
      }
      pthread_mutex_unlock(&(sharedResources->mutex));
      printf("Path %s current Manager: %d, newManager: %d\n", newPath,
             newNManager, newNWorker);
    } else {
      printInfo("Invalid Argument passed from input, syntax must "
                "be:\n-Path\n-Number of Manager\n-Number of Worker\n");
    }

    // TODO... Check for other failure
    if (rc_ct == SUCCESS) {
      printf("Quanto vale: %s\n", newPath);
      type = precomputeAnalyzerInput(tmpCwd, newPath, &rc_an);
      if (rc_an != SUCCESS) {
        if (rc_an != EMPTY_PATH) {
          printError("Error inside precompute analyzer Input");
          rc_t = rc_an;
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
            printf("Lo pusho\n");
            push(sharedResources->candidateNode, toAnalyze);
            // printList(sharedResources->candidateNode, printCandidateNode);
          } else {
            rc_t = MALLOC_FAILURE;
          }
          pthread_mutex_unlock(&(sharedResources->mutex));
        } else {
          // TODO... IL signore va gestito con l'error handler
          if (type == NOT_EXISTING) {
            char *msgInfo = (char *)malloc(sizeof(char) * (PATH_MAX + 300));
            rc_al5 = checkAllocationError(msgInfo);
            if (rc_al5 < 0) {
              // TODO... HARAKIRI PROBABLY
              printError("I can't allocate memory");
              rc_t = MALLOC_FAILURE;
            } else {
              sprintf(msgInfo, "The File %s doesn't exist", newPath);
              printInfo(msgInfo);
              free(msgInfo);
            }
          } else {
            printInfo("I didn't understand the file type of what you gave me "
                      "(bash call error)");
          }
        }
      }
    }
    usleep(500);
  }
  printf("sono morto in read directives loop: %d\n", rc_t);
  free(newPath);
  free(nWorker);
  free(nManager);
  free(tmpCwd);
  // kill(getpid(), SIGKILL);
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

int parentInitExecPipe(const int toParent[], const int toChild[]) {
  int rc_t = 0;
  int rc_cl = closeDescriptor(toParent[WRITE_CHANNEL]);
  int rc_cl2 = closeDescriptor(toChild[READ_CHANNEL]);

  if (rc_cl == -1 || rc_cl2 == -1) {
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
      // printf("free newManager\n");
      free(manager->filesInExecution);
      free(manager);
      // printf("dopo free newManager\n");
    } else {
      manager->pipe = (int *)malloc(sizeof(int) * 2);
      rc_ca = checkAllocationError(manager->pipe);
      if (rc_ca != SUCCESS) {
        // printf("free newManager2\n");
        free(manager->filesInExecution);
        free(manager);
      }
    }
  }
  return manager;
}

int addManagers(PriorityQueue managers, int amount) {
  int rc_t = SUCCESS;
  int rc_en = SUCCESS;
  int i = 0;
  if (managers != NULL) {
    for (i = 0; i < amount && rc_t == SUCCESS; i++) {
      // TODO... create manager struct
      Manager manager = newManager();
      if (manager == NULL)
        rc_t = NEW_MANAGER_FAILURE;
      else {
        rc_en = pushPriorityQueue(managers, manager->filesInExecution->size,
                                  (void *)manager);
        // rc_en = push(managers, manager);
        if (rc_en == -1)
          rc_t = MALLOC_FAILURE;
        else {
          int toParent[2];
          int toChild[2];
          // TODO... create pipe
          createUnidirPipe(toParent);
          createUnidirPipe(toChild);
          int managerPid = fork();
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
            execlp("./manager", "./manager", NULL);
            printf("Se muoio qua faccio cagare add Managers\n");
            kill(getpid(), SIGKILL);
          }
        }
      }
    }
  } else {
    rc_t = MALLOC_FAILURE;
  }
  return rc_t;
}

/**
 * Naive solution. It is not the best solution, but it is cheap
 * The function will assign
 */
int endManager(Manager m, List fileToAssign) {
  int rc_t = SUCCESS;
  if (fileToAssign != NULL) {
    if (isEmptyList(fileToAssign)) {
      swap(fileToAssign, m->filesInExecution);
    } else {
      concat(fileToAssign, m->filesInExecution);
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
  // printf("free destroyManager\n");
  free(manager->pipe);
  free(manager->filesInExecution);
  free(manager);
  // printf("dopo free destroyManager\n");
}

int removeManagers(PriorityQueue managers, int amount, List fileToAssign) {
  int rc_t = SUCCESS;
  while (amount != 0 && managers->size != 0) {
    Manager m = popPriorityQueue(managers);
    if (m != NULL) {
      // int rc_po = pop(managers);
      endManager(m, fileToAssign);
      kill(m->m_pid, SIGKILL);
      destroyManager((void *)m);
      amount--;
    } else {
      rc_t = REMOVE_MANAGER_FAILURE; // VEDERE COME GESTIRE
    }
  }
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
      // TODO... add check for sprintf (wrapping function)
      int rc_sp1 = sprintf(nworkers, "%d", currentWorker);
      // TODO... write m in pipe to manager
      if (rc_sp1 > 0) {
        // printf("sprintf ok\n");
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
    rc_t = ASSIGNFILE_MEMORY_FAILURE;
  return rc_t;
}

int isManagerAlive(Manager m) {
  int rc_t = SUCCESS;
  int returnCode = kill(m->m_pid, 0);
  if (returnCode != 0) {
    rc_t = DEAD_PROCESS;
  }
  // TODO non deve farlo
  return SUCCESS;
}

int rescheduleFiles(List toDestroy, List toReschedule) {
  int rc_t = SUCCESS;
  if (isEmptyList(toReschedule) == EMPTY) {
    if (isEmptyList(toDestroy) != EMPTY) {
      rc_t = swap(toDestroy, toReschedule);
    }
  } else {
    TreeNode file = NULL;
    while (isEmptyList(toDestroy) == NOT_EMPTY && rc_t == SUCCESS) {
      file = (TreeNode)front(toDestroy);
      rc_t = pop(toDestroy);
      if (rc_t == SUCCESS) {
        rc_t = enqueue(toReschedule, (void *)file);
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
  int nManager = 0;
  int isAliveM = 0;
  int found = 0;
  int *pipe = NULL;
  char *path = (char *)malloc(sizeof(char) * PATH_MAX);
  char *number = (char *)malloc(sizeof(char) * PATH_MAX);
  // TODO... contorllare il loro fallimento
  char charRead = 'a';
  // TODO... da cambiare con una define
  int numbersToRead = NCHAR_TABLE;
  int bytesRead = 1;
  int counter = 0;
  int insertCounter = 0;
  unsigned long long charCounter = 0;
  // TODO... magari una define anche per questa?
  char *controlWord = (char *)malloc(sizeof(char) * CONTROL_WORD_LEN);
  Manager manager = NULL;
  finished = newList();
  PriorityQueue tmpManagers = newPriorityQueue();
  int rc_ca = checkAllocationError(path);
  int rc_ca2 = checkAllocationError(number);
  int rc_ca3 = checkAllocationError(controlWord);
  if (tmpManagers == NULL || rc_ca != SUCCESS || rc_ca2 != SUCCESS ||
      rc_ca3 != SUCCESS) {
    rc_t = MALLOC_FAILURE;
  }
  unsigned long long accumulator = 0;
  unsigned long long accumulatorNUMBER = 0;
  while (rc_t == SUCCESS) {
    // TODO... check if this type of approach is blocking or not
    pthread_mutex_lock(&(sharedResources->mutex));
    nManager = *(sharedResources->nManager);
    //pthread_mutex_unlock(&(sharedResources->mutex));
    while (nManager > 0 && rc_t == SUCCESS) {
      //pthread_mutex_lock(&(sharedResources->mutex));
      // printf("priorityQueue size %d\n", sharedResources->managers->size);
      manager = popPriorityQueue(sharedResources->managers);
      //pthread_mutex_unlock(&(sharedResources->mutex));
      if (manager != NULL) {
        isAliveM = isManagerAlive(manager);
        if (isAliveM == SUCCESS) {
          pipe = manager->pipe;
          bytesRead = read(pipe[READ_CHANNEL], &charRead, 1);
          // printf("sono bloccato, bytesRead %d\n", bytesRead);
          if (bytesRead > 0) {
            // printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
            while (charRead != 0 && isManagerAlive(manager) == SUCCESS) {
              if (bytesRead > 0) {
                path[counter] = charRead;
                counter++;
              }
              bytesRead = read(pipe[READ_CHANNEL], &charRead, 1);
            }
            path[counter] = 0;
            //printf("--------------------- ANALYZER %d: path ricevuto %s\n", getpid(), path);
            //pthread_mutex_lock(&(sharedResources->mutex));
            node = manager->filesInExecution->head;
            found = 0;
            while (node != NULL && found == 0 && rc_t == SUCCESS) {
              file = (TreeNode)node->data;
              if (file != NULL) {
                info = (FileInfo)file->data;
                if (info != NULL) {
                  if (strcmp(path, info->path) == 0) {
                    // printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
                    // pasta(node->data);
                    // pasta(file);
                    // printf("guarda come non mi rompo: %s\n", info->path);
                    // printf("BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB\n");
                    found = 1;
                  } else {
                    node = node->next;
                  }
                } else {
                  printf("MUIO PER UN INFO DI FILE NULLO\n");
                  rc_t = NULL_POINTER;
                }
              } else {
                printf("MUIO PER UN TREENODE NULLO\n");
                rc_t = NULL_POINTER;
              }
            }
            //pthread_mutex_unlock(&(sharedResources->mutex));

            if (found != 1 || rc_t != SUCCESS) {
              printf("MUIO PERCHE' NON TROVO QUELLO CHE STAVO CERCANDO\n");
              rc_t = FILE_NOT_FOUND;
            } else {
              insertCounter = 0;
              numbersToRead = NCHAR_TABLE;
              accumulatorNUMBER = 0;
              while (numbersToRead > 0 && isManagerAlive(manager) == SUCCESS) {
                counter = 0;
                charRead = 'a';
                while (charRead != 0 && isManagerAlive(manager) == SUCCESS) {
                  bytesRead = read(pipe[READ_CHANNEL], &charRead, 1);
                  if (bytesRead > 0) {
                    // printf("charRead in number: %c, %d\n", charRead,
                    // charRead); usleep(100000);
                    number[counter] = charRead;
                    counter++;
                  }
                }
                number[counter] = 0;
                /* printf("%s ", number);
                fflush(stdout);*/
                // usleep(10000);
                // Posso fare l'inserimento
                rc_ss = sscanf(number, "%llu", &charCounter);
                unsigned long long ab = 0;
                int a = sscanf(number, "%llu", &ab);
                //printf("%s %llu\n", number, ab);
                //fflush(stdout);
                accumulatorNUMBER += ab;
                if (rc_ss > 0) {
                  //pthread_mutex_lock(&(sharedResources->mutex));
                  info->fileTable[insertCounter] = charCounter;
                  // printf("NUMERO %llu\n", info->fileTable[insertCounter]);
                  insertCounter++;
                  //pthread_mutex_unlock(&(sharedResources->mutex));
                } else {
                  printf("fail della scanf\n");
                  // TODO vedere che fare in caso di fallimento di sscanf
                }
                number[0] = 0;
                numbersToRead--;
              }
              counter = 0;
              if (isManagerAlive(manager) == SUCCESS) {
                // TODO... CHECK IF FILE IS FINISHED OR NOT
                bytesRead =
                    read(pipe[READ_CHANNEL], controlWord, CONTROL_WORD_LEN);
                // printf("parola di controllo: %s\n", controlWord);
                if (bytesRead > 0) {
                  if (strcmp(controlWord, CONTROL_DONE) == 0) {
                    printf("Done!!!\n");
                    accumulator++;
                    printf("Acc: %llu\n", accumulator);
                    printf("NUMBER ACC: %llu\n", accumulatorNUMBER);
                    //pthread_mutex_lock(&(sharedResources->mutex));
                    //printf("ANALYZER %d: prima\n", getpid());
                    //printList(manager->filesInExecution, pasta);
                    //toStringTable(info->fileTable);
                    //pasta(file);
                    // pasta(node->data);
                    push(finished, info);
                    //printList(finished, pasta);
                    rc_t = detachNodeFromList(manager->filesInExecution, node);
                    printf("HO SIZE: %d %d\n", manager->filesInExecution->size, manager->m_pid);
                    //printf("ANALYZER %d: dopo\n", getpid());
                    // usleep(100000);
                    //printList(manager->filesInExecution, pasta);
                    // fflush(stdout);
                    // sleep(5);
                    //pthread_mutex_unlock(&(sharedResources->mutex));
                  } else if (strcmp(controlWord, CONTROL_UNDONE) == 0) {
                    // printf("UNDONE!\n");
                  } else {
                    printf("Nn o kpt una kppa\n");
                  }
                }
              } else {
                //pthread_mutex_lock(&(sharedResources->mutex));
                rc_em = endManager(manager, sharedResources->fileToAssign);
                if (rc_em < SUCCESS) {
                  rc_t = rc_em;
                  printError("Error in save Manager Work\n");
                }
                printf("DISTRUGGO LO STRONZO pt 1\n");
                destroyManager((void *)manager);
                //pthread_mutex_unlock(&(sharedResources->mutex));
              }
            }
          }
          //pthread_mutex_lock(&(sharedResources->mutex));
          pushPriorityQueue(tmpManagers, manager->filesInExecution->size,
                            manager);
          //pthread_mutex_unlock(&(sharedResources->mutex));
        } else {
          //pthread_mutex_lock(&(sharedResources->mutex));
          printf("e' morto quello con pid %d\n", manager->m_pid);
          // rc_em = endManager(manager, sharedResources->fileToAssign);
          /*if(rc_em < SUCCESS){
              rc_t = rc_em;
              printError("Error in save Manager Work\n");
          }*/
          printf("DISTRUGGO LO STRONZO pt 2\n");
          // destroyManager((void *) manager);
          //pthread_mutex_unlock(&(sharedResources->mutex));
        }
      } else {
        printf("MANAGER NULLO\n");
        rc_t = NULL_POINTER;
      }
      nManager--;
      // TODO STAMPA COME WORD COUNTER
    }
    //pthread_mutex_lock(&(sharedResources->mutex));
    rc_sw = swapPriorityQueue(sharedResources->managers, tmpManagers);
    if (rc_sw != SUCCESS) {
      printf("errore swap ciaone\n");
      rc_t = rc_sw;
    }
    //pthread_mutex_unlock(&(sharedResources->mutex));
    // sleep(2);
    if (isEmptyList(sharedResources->fileToAssign) == NOT_EMPTY) {
      rc_mfs = manageFileToSend(sharedResources->managers,
                                *(sharedResources->nWorker),
                                sharedResources->fileToAssign);
    }
    pthread_mutex_unlock(&(sharedResources->mutex));
    // TODO... Handle with an handler for more specific errors if(rc_mfs ==
    // something) then ...
    usleep(1);
  }
  printf("SEND FILE LOOP rc_t %d\n", rc_t);
  fflush(stdout);
  sleep(1);
  // kill(getpid(), SIGKILL);
}

void sighandle_print(int sig){
  //printList(finished, pasta2);
  int fd = open("/tmp/conteggio.txt", O_WRONLY);
  // printf("ACCUMULATORE %llu\n", accumulator);
  printf("Scorro tutto\n");
  while(isEmptyList(finished) == NOT_EMPTY){
      //printf("SONO UN FIGLIO DI TROIA\n");
      FileInfo finfo = front(finished);
      pop(finished);
      unsigned long long counterSchifo = 0;
      int i = 0;
      for(i = 0; i < NCHAR_TABLE; i++){
          counterSchifo += finfo->fileTable[i];
      }
      char *stringa = calloc(PATH_MAX + INT_MAX_LEN + 1, sizeof(char));
      sprintf(stringa, "%s;%llu\n", finfo->path, counterSchifo);
      write(fd, stringa, strlen(stringa) + 1);
      free(stringa);
  }
  close(fd);
  printf("Ho finito di scrivere\n");
  signal(SIGINT, SIG_DFL);
}

int manageFileToSend(PriorityQueue managers, int currentWorker,
                     List filesToAssign) {
  int rc_t = SUCCESS;
  int rc_sf = SUCCESS;
  int rc_po = SUCCESS;
  int rc_pu = SUCCESS;
  int rc_em = SUCCESS;
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
              rc_t = rc_pu;
            } else {
              rc_t = rc_sf;
            }
          } else {
            rc_t = ASSIGNWORK_FAILURE;
          }
        }
      } else {
        // TODO... change behavior
        rc_em = endManager(m, filesToAssign);
        if (rc_em < SUCCESS) {
          rc_t = rc_em;
          printError("Error in save Manager Work\n");
        }
        destroyManager((void *)m);
        // rc_t = DEAD_PROCESS;
      }
    }
  } else {
    rc_t = NULL_POINTER;
  }
  //}
  // printf("esco da manage file to send\n");
  return rc_t;
}

/**
 * Function that init the fileInfo root of the tree
 */
// Ho spostato la funzione, speriamo bene che tutto funzioni correttametne
FileInfo initFileInfoRoot(int *msg) {
  FileInfo rootData;
  char *rootName = (char *)malloc(sizeof(char) * 2);
  int rc_ca = checkAllocationError(rootName);
  int rc_nc = SUCCESS;
  if (rc_ca < 0) {
    // TODO... HARAKIRI PROBABLY
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

/**
 * Function that init the analyzer tree
 */
Tree initializeAnalyzerTree(FileInfo data, int *msg, void destroyer(void *)) {
  Tree tree = NULL;
  tree = newTree((void *)data, msg, destroyer, NULL);
  if (*msg != SUCCESS) {
    char *msgErr = (char *)malloc(300);
    int rc_ca = checkAllocationError(msgErr);
    if (rc_ca < 0) {
      // TODO... HARAKIRI PROBABLY
      printError("I can't allocate memory");
    } else {
      sprintf(msgErr, "Can't create tree inside Analyzer: %d", getpid());
      printInfo(msgErr);
      free(msgErr);
    }
  }
  return tree;
}

/**
 * Function that obtains the current working directory
 */
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
      *childPid = f;
    } else if (f == 0) {
      // TODO... Deal with pipe
      rc_cds_1 = closeDescriptor(fd[READ_CHANNEL]);
      rc_cdp = createDup(fd[WRITE_CHANNEL], 1);
      rc_cds_2 = closeDescriptor(fd[WRITE_CHANNEL]);
      rc_exlp = execlp("find", "find", compactedPath, "-mindepth", "1",
                       "-readable", "-type", "f", NULL);
      printf("MUOIO CON LA FIND\n");
      // kill(getpid(), SIGKILL);
      // printf("fallisco miseramente\n");
    } else {
      printError("The program couldn't execute a fork");
      rc_t = FORK_FAILURE;
    }
  } else {
    rc_t = PIPE_FAILURE;
    printError("Pipe find in saveFindResult gone wrong");
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
    rc_t = MALLOC_FAILURE;
  }

  while (rc_t == SUCCESS) {
    if (readFlag != SUCCESS) {
      pthread_mutex_lock(&(sharedResources->mutex));
      // printf("size lista sharedResources %d\n",
      // sharedResources->candidateNode->size);
      if (isEmptyList(sharedResources->candidateNode) == NOT_EMPTY) {
        candidate = (TreeNodeCandidate)front(sharedResources->candidateNode);
        int rt_po = pop(sharedResources->candidateNode);
        if (candidate == NULL || rt_po < SUCCESS) {
          rc_t = UNEXPECTED_LIST_ERROR;
        } else {
          printf("Setto insertFlag a true dovuto a %s, type %d, skip %d\n",
                 candidate->path, candidate->type, candidate->toSkip);
          insertFlag = SUCCESS;
        }
      }
      pthread_mutex_unlock(&(sharedResources->mutex));
    } else {
      insertFlag = FAILURE;
    }

    // Parte di inserimento
    if (insertFlag == SUCCESS) {
      printf("Precompute ok!!\n");
      childPid = 0;
      if (candidate->type == DIRECTORY) {
        rc_find = spawnFindProcess(candidate->path, fd, &childPid);
        printf("Lo ho spawnato: %d - %d, %d\n", fd[READ_CHANNEL], fd[WRITE_CHANNEL], childPid);
        readFlag = 1;
        if (rc_find != SUCCESS) {
          printError("Error in spawn find result");
          // TODO... NON FARE NULLA -> Need to be Handled in some way
        } else {
          readFlag = SUCCESS;
        }
      } else {
        skipPath(candidate->path, relativePath, candidate->toSkip);
        pthread_mutex_lock(&(sharedResources->mutex));
        toSchedule = performInsert(relativePath, candidate->path,
                                   candidate->startingNode, IS_FILE, &rc_pi);
        // TODO... VEDERE L'ERRORE, vedere soprattutto il fatto dell'inserimento
        // doppio
        printf("sono un file\n");
        if (rc_pi == SUCCESS) {
          rc_en = SUCCESS;
          rc_en = enqueue(sharedResources->fileToAssign, (void *)toSchedule);
          if (rc_en != SUCCESS) {
            printError("Error in assign file to manager");
            rc_t = rc_en;
          }
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
          // printf("charRead in find: %c, %d\n", charRead, charRead);
          // usleep(100000);
          if (skipped >= candidate->toSkip) {
            if (charRead == '\n' || charRead == '\0') {
              relativePath[counter] = '\0';
              candidate->path[counter + skipped] = '\0';
              if (counter > 0) {
                //printf("relativePath: %s, candidate->path: %s\n", relativePath, candidate->path);
                // usleep(100000);
                pthread_mutex_lock(&(sharedResources->mutex));
                rc_ia = insertAndSchedule(candidate->startingNode,
                                          sharedResources->fileToAssign,
                                          relativePath, candidate->path);
                pthread_mutex_unlock(&(sharedResources->mutex));
                if (rc_ia != SUCCESS) {
                  // TODO... handle if file already inserted or malloc failure
                  printf("rc_ia %d\n", rc_ia);
                  printError("Insert and schedule failed");
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
        } else {
          //! Strange case due to the scheduler: Thanks scheduler
          // printf("infamata assurda!\n");
        }

      } else {
        closeDescriptor(fd[READ_CHANNEL]);
        readFlag = FAILURE;
        destroyTreeNodeCandidate(candidate);
        // printf("Ho finito di leggere perche': %d\n", bytesRead);
      }
    }
    usleep(500);
  }
  printf("Ho rct %d perche' sono morto FILEMANAGE \n", rc_t);
  free(relativePath);
  // kill(getpid(), SIGKILL);
}

Tree fs = NULL;

/**
 * Signal handler for debug

void sighandle_int(int sig){
    destroyTree(fs);
    signal(SIGINT, SIG_DFL);
}
*/

int main() {
  signal(SIGCHLD, SIG_IGN);
  signal(SIGINT, sighandle_print);
  sharedResourcesAnalyzer_t sharedResources;
  // Tree fs = NULL;
  TreeNode currentDirectory = NULL;
  FileInfo root = NULL;
  // TODO check for literals, ehm... I mean errors!
  PriorityQueue managers = newPriorityQueue();
  List fileToAssign = newList();
  List candidateNode = newList();
  char cwd[PATH_MAX];
  char *path = (char *)malloc(sizeof(char) * PATH_MAX);
  int rc_al = checkAllocationError(path);
  int defaultManagers = 3;
  int defaultWorkers = 4;
  int rc_cwd = SUCCESS;
  int rc_ir = SUCCESS;
  int rc_it = SUCCESS;
  int rc_pi = SUCCESS;
  int rc_t = SUCCESS;
  int rc_am = SUCCESS;
  // fprintf(stderr, "OOOOOOOO\n");
  rc_am = addManagers(managers, defaultManagers);
  rc_cwd = getCwd(cwd);
  // fprintf(stdout, "Sono nel main!\n");
  // Alla fine tutto  printError verranno sostituiti con un handler dell'errore,
  // quindi si, sono brutti, ma diciamo che è rimediabile
  if (rc_cwd == SUCCESS && rc_am == SUCCESS && managers != NULL &&
      rc_al == SUCCESS && candidateNode != NULL && fileToAssign != NULL) {
    // CANNOT MOVE THIS CODE (AT LEAST IF YOU DON'T WANT TO DEAL WITH COPY OF
    // POINTER)
    root = initFileInfoRoot(&rc_ir);
    if (rc_ir == SUCCESS) {
      // Inizializzazione dell'albero
      fs = initializeAnalyzerTree(root, &rc_it, destroyFileInfo);
      if (rc_it == SUCCESS) {
        currentDirectory =
            performInsert(cwd, NULL, getRoot(fs), DIRECTORY, &rc_pi);
        if (rc_pi != SUCCESS) {
          rc_t = rc_pi;
          printError("Unable to insert the current directory :(");
        }
      } else {
        rc_t = rc_it;
        printError("I am not able to initialize the root of the tree, that's "
                   "terrible guys");
      }
    } else {
      rc_t = rc_ir;
      printError(
          "I am not able to allocate the root of the tree, this is very bad");
    }
    // Guarda che casinoooooooo
    sharedResources.cwd = cwd;
    sharedResources.fileToAssign = fileToAssign;
    sharedResources.currentDirectory = currentDirectory;
    sharedResources.fs = fs;
    sharedResources.managers = managers;
    sharedResources.nManager = &defaultManagers;
    sharedResources.nWorker = &defaultWorkers;
    sharedResources.path = path;
    sharedResources.candidateNode = candidateNode;
    pthread_t reads;
    pthread_mutex_init(&sharedResources.mutex, NULL);
    pthread_create(&reads, NULL, readDirectivesLoop, (void *)&sharedResources);
    pthread_t fileManage;
    pthread_create(&fileManage, NULL, fileManageLoop, (void *)&sharedResources);
    pthread_t send;
    pthread_create(&send, NULL, sendFileLoop, (void *)&sharedResources);
    printf("NON DEVO MORIRE\n");
    pthread_join(reads, NULL);
    pthread_join(fileManage, NULL);
    pthread_join(send, NULL);
    printf("free tree\n");
    destroyTree(fs);
    printf("dopo free tree\n");
  } else {
    // TODO... Create handler for these errors
    if (rc_am != SUCCESS) {
      rc_t = rc_am;
    } else {
      printError("I am not able to find the current working directory. This is "
                 "a disgrace");
      rc_t = CWD_ACCESS_DENIED;
    }
  }
  // TODO... destroyList(fileToAssign, destructor);
  printf("free manager\n");
  destroyPriorityQueue(managers, destroyManager);
  printf("dopo free manager\n");
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
