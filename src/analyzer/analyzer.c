#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <limits.h>
#include "./analyzer.h"
#include "../tree/tree.h"
#include "../manager/manager.h"
#include "../table/table.h"

#define READ 0
#define WRITE 1

#define RELATIVE 0
#define ABSOLUTE 1

#define GOOD_ENDING 0
#define BAD_ENDING -1

#define SUMMARY 0
#define NEW_DIRECTIVES 1

#define HARAKIRI -1

#define OK 0
#define WORKING 1
#define NEW_WORKER_FAILURE -1
#define TABLE_FAILURE -2
#define WORK_FAILURE -3
#define READ_FAILURE -4
#define END_WORK_FAILURE -5
#define NEW_DIRECTIVES_FAILURE -6
#define CAST_FAILURE -7
#define PIPE_FAILURE -8
#define SUMMARY_FAILURE -9
#define REMOVE_WORK_FAILURE -10
#define INIT_FAILURE -11
#define ASSIGNWORK_FAILURE -12
#define SEND_FAILURE -13
#define DESCRIPTOR_FAILURE -14
#define ASSIGNWORK_MEMORY_FAILURE -15
#define DEAD_PROCESS -16
#define START_NEW_MANAGER -17
#define NEW_MANAGER_FAILURE -18
#define FORK_FAILURE -19
#define MANAGER_NULL_ERORR -20
#define ENQUEUE_MANAGER_ERROR -21
#define FILE_NULL_POINTER -22
#define CWD_FAILURE -23
#define STARTING_DIRECTORY_FAILURE -24
#define ASSIGNFILE_MEMORY_FAILURE -25

#define MAXLEN 300
#define NCHAR_TABLE 128
#define WRITE_CHANNEL 1
#define READ_CHANNEL 0

#define DIRECTORY 1
#define FILE 0
#define NOT_EXISTING 2

#define FAILURE -1
#define SUCCESS 0

/**
 * Destructor of the file info structure
 */
void destroyFileInfo(void *toDestroy){
    FileInfo data = (FileInfo) toDestroy;
    printf("currently destroying %s\n", (char *)(data->name));
    free(data->name);
    free(toDestroy);
}

/**
 * Function that check for strcpy
 */
int checkStrcpy(char *dst, char *src){
    int rc_cp = SUCCESS;
    if(strcpy(dst, src) == NULL){
        rc_cp = FAILURE;   
    }
    return rc_cp;
}

/**
 * Function that allocate path and check if it return a failure
 */
int allocatePath(FileInfo fileinfo, char *string){
    int rc_a = SUCCESS;
    if(string != NULL){
        fileinfo->path = (char *)malloc(sizeof(char)*PATH_MAX);
        rc_a = checkAllocationError(fileinfo->name);
        if(rc_a == SUCCESS){
            rc_a = checkStrcpy(fileinfo->path, string);
        }else{
            printf("Muoio sempre in allocate path 111\n");
        }
    }else{
        fileinfo->path = NULL;
    }
    return rc_a;
}

/**
 * Costructor of the file info structure
 */
FileInfo newFileInfo(char *name, int isDirectory, char *path, int *msg){
    FileInfo toRtn = (FileInfo) malloc(sizeof(struct FileInfo));
    *msg = checkAllocationError(toRtn);
    int rc_ca = SUCCESS;
    if(*msg == SUCCESS){
        toRtn->name = name;
        toRtn->isDirectory = isDirectory;
        *msg = allocatePath(toRtn, path);
    }
    return toRtn;
}

/**
 *  File info toString
 */
void pasta(void *data){
    printf("File : %s name %s isFolder %d\n", ((FileInfo)((TreeNode)data)->data)->path, ((FileInfo)((TreeNode)data)->data)->name, ((FileInfo)((TreeNode)data)->data)->isDirectory);
}

/**
 * Manager toString function
 */
void toStringManager(void *data){
    Manager manager = (Manager) data;
    printList(manager->filesToAssign, pasta);
}

/**
 * Scan the children list of a file for matches of the current path position and the name of the directory that compose the path of a file/folder
 */
void scanNodeList(FileInfo dataToExamine, char *path, int *counter, int *tmpCounter, int *resetCounter, int *match){
    //printf("controllo %s, tmpCounter = %d\n", dataToExamine->name, *tmpCounter);
    while(path[*counter] != '/' && path[*counter] != '\0' && *match == 0){
        if(path[*counter] == dataToExamine->name[*tmpCounter]){
            *counter = *counter + 1;
            *tmpCounter = *tmpCounter + 1;
            //printf("\tmatch carattere\n");
        } else {
            //printf("\tnon faccio match per colpa di %c che dovrebbe essere %c\n", dataToExamine->name[tmpCounter], path[counter]);
            *counter = *resetCounter;
            *tmpCounter = 0;
            *match = -1;
        }
    }
}

/**
 * Function that handle the case of match:
 *  - file already inserted
 *  - false positive, the node contains the path but they're different
 */
Node handleMatch(TreeNode toExamine, FileInfo dataToExamine, Node actualNode, char*path, int *counter, int *tmpCounter, int *resetCounter, int *found, int *match){
    //MATCH = 0
    Node toRtn = NULL;
    if(dataToExamine->name[*tmpCounter] == '\0'){ //Two names matches perfectly
        //printf("MATCH\n");
        if(path[*counter] == '\0'){ //It's already a folder or a file in the actual tree no need to insert again
            *found = -1;
            printf("File Already Inserted\n");
        } else {
            if(isEmptyList(toExamine->children) == NOT_EMPTY){ //If I have some inner files then we need to check if we can continue to have a match
                toRtn = toExamine->children->head;
            } else { //I've arrived at the deepest known part in the tree which has already been inserted in the tree
                *found = 1;
            }
            *resetCounter = *counter + 1;
            *counter = *resetCounter;
            *tmpCounter = 0;
        }
    } else { //The children name contains the name in the path but they're different
        *counter = *resetCounter;
        //*tmpCounter = 0;
        *match = -1;
        //printf("I'm a false positive\n");
    }
    //printf("Sono rottissimo parte 3: %d\n", *found);
    return toRtn;
}

/**
 * Perform the research of the file by scanning the child node and handle the case that file's path and tree search path matches 
 * If there is no match it sets the current search node to the next node
 */
Node performResearch(TreeNode toExamine, Node actualNode, char *path, int *counter, int *tmpCounter, int*found, int *match){
    //"carrefour" with actualNode
    Node toRtn = NULL;
    int resetCounter = 0;
    FileInfo dataToExamine = NULL;
    if(toExamine != NULL){
        dataToExamine = (FileInfo) toExamine->data;
        if(dataToExamine != NULL && dataToExamine->name != NULL){
            scanNodeList(dataToExamine, path, counter, tmpCounter, &resetCounter, match);
            if(*match == 0){
                toRtn = handleMatch(toExamine, dataToExamine, actualNode, path, counter, tmpCounter, &resetCounter, found, match);
                printf("Faccio match\n");
            }
        }
    }
    if(*match != 0){ //if I didn't match  then i need to look if a brother of mine has maybe the folder/file I'm looking for
        toRtn = actualNode->next;
    }
    return toRtn;
}

/**
 * Function that performs the insert of the fileInfo
 * args: 
 *      char *path: path to insert
 *      TreeNode startingPoint: where to start the insertion
 *      int isDirectory: 1 if the file which will be inserted is a directory, 0 otherwise
 *      int *msg: a message if an errror occours
 * returns:
 *      if a directory is inserted the corrsiponding pointer of the node will be returned, otherwise it returns null
 * */

TreeNode decideWhereToAttach(TreeNode startingPoint, char *path, int *counter, int*found){
    TreeNode toExamine = NULL;
    Node actualNode = NULL;
    int match = 0;
    int tmpCounter = 0;
    if(isEmptyList(startingPoint->children) == NOT_EMPTY){
        actualNode = startingPoint->children->head;
        while(*found == 0 && actualNode != NULL){
            printf("size: %d\n", startingPoint->children->size);
            match = 0;
            toExamine = (TreeNode) actualNode->data;
            printf("%d\n", toExamine);
            //Ocio ad actual Node -> probabilità di esplosione stimata 87/100 (se si passare per riferimento)
            //QUI//actualNode = performResearch(toExamine, actualNode, path, counter, &tmpCounter, found, &match);
            //printf("Sono rottissimo: %d\n", *found);
        }
    }
    return toExamine;
}

/**
 * Function that handle the case if the funtction is found
 */
TreeNode setWhereToInsert(TreeNode startingPoint, TreeNode toExamine, int *found){
    TreeNode whereToInsert = NULL;
    if(*found == 0){ //The new Element wasn't a children of mine then i need to add it to my children
        if(toExamine == NULL){ //I need to attach the children to the Starting Point
            //printf("Qui\n");
            whereToInsert = startingPoint;
        } else { //The new node is my brother so I need to attach the children to my father
            //printf("Quo\n");
            whereToInsert = toExamine->parent;
        }
    } else if (*found == 1){ //The element it's my first child (Probably never going to happen buuuuuut... YNK)
        printf("Qua\n");
        whereToInsert = toExamine;
    }
    //printf("Lmao vale %d\n", (whereToInsert == NULL) ? 1 : 0);
    return whereToInsert;
}

/**
 * Function that create and links the file to the where to insert node
 */
TreeNode createAndLinkFile(TreeNode whereToInsert, char *pathName, char *completePath, int *tmpCounter, const int isDirectory, int *msg){
    TreeNode toInsert = NULL;
    FileInfo dataToInsert = NULL;
    int rc_nc = SUCCESS;
    int rc_tc = SUCCESS;
    pathName[*tmpCounter] = '\0';
    //printf("file: %s\n", pathName);
    dataToInsert = newFileInfo(pathName, isDirectory, completePath, &rc_nc);
    toInsert = newTreeNode(whereToInsert, (void *) dataToInsert, &rc_tc);
    if(rc_nc == SUCCESS && rc_tc == SUCCESS){
        linkChild(whereToInsert, toInsert);
    } else {
        free(dataToInsert);
        free(toInsert);
        *msg = MALLOC_FAILURE;
    }
    return toInsert;
}

/**
 * Function that create and link directory to the toInsert node
 */
TreeNode createAndLinkDirectory(TreeNode whereToInsert, char *pathName, int *tmpCounter){
    TreeNode toInsert = NULL;
    FileInfo dataToInsert = NULL;
    int rc_t = SUCCESS;
    int rc_nc = SUCCESS;
    int rc_tc = SUCCESS;
    int rc_ca = SUCCESS;
    pathName[*tmpCounter] = '\0';
    dataToInsert = newFileInfo(pathName, 1, NULL, &rc_nc);
    toInsert = newTreeNode(whereToInsert, (void *) dataToInsert, &rc_tc);
    //printf("coide ritorno data %d e insert %d\n", rc_nc, rc_tc);
    if(rc_nc == SUCCESS && rc_tc == SUCCESS){
        linkChild(whereToInsert, toInsert);
        pathName = (char *)malloc(sizeof(char)*PATH_MAX);
        rc_ca = checkAllocationError(pathName);
        if(rc_ca != SUCCESS){
            printError("Non sono riuscito ad allocare path name in create and link directory");
            rc_t = rc_ca;
        }
        //printf("coide ritorno: %d\n", rc_ca);
        *tmpCounter = 0;
        whereToInsert = toInsert;

    } else {
        free(dataToInsert);
        free(toInsert);
        rc_t = FAILURE;
    }
    return whereToInsert;
}

/**
 * Function that insert a TreeNode into the tree
 * It handles the fact that it is a file or a directory
 */
TreeNode insertDataIntoTree(TreeNode whereToInsert, char *path, char* completePath, int *counter, const int isDirectory, int *msg){
    TreeNode parentNode;
    //printf("Pasta di insertDataIntoTree: \n");
    //pasta(whereToInsert);
    printf("path: %s, complete path: %s, counter %d, isDirectory: %d\n", path, completePath, *counter, isDirectory);
    int rc_t = SUCCESS;
    int rc_calf = SUCCESS;
    int rc_cald = SUCCESS;
    int rc_ca = SUCCESS;
    int tmpCounter = 0;
    printf("sono il padre del nodo che stai analizzando e sono %s\n", ((FileInfo)whereToInsert->data)->name);
    //printf("size: %d\n", whereToInsert->children->size);
    //EFFICENCY OVER THE MEMORY OVER NINE THOUSAND
    char* pathName = (char *)malloc(sizeof(char)*PATH_MAX);
    rc_ca = checkAllocationError(pathName);
    if(rc_ca == SUCCESS){
        while(path[*counter]!='\0' && rc_ca == SUCCESS){
            //printf("giro nel ciclo\n");
            if(path[*counter] == '/'){
                if(*counter != 0){ //No double root insert
                    //printf("Inserisco in %s\n", ((FileInfo)whereToInsert->data)->name);
                    whereToInsert = createAndLinkDirectory(whereToInsert, pathName, &tmpCounter);
                    //printf("e ora divento %s\n", ((FileInfo)whereToInsert->data)->name);
                    if(whereToInsert == NULL){
                        printError("Error in create and link directory");
                        rc_t = MALLOC_FAILURE;
                    }
                }
            } else {
                pathName[tmpCounter] = path[*counter];
                tmpCounter = tmpCounter + 1;
            }
            *counter = *counter + 1;
        }
        if(rc_ca == SUCCESS){
           parentNode = createAndLinkFile(whereToInsert, pathName, completePath, &tmpCounter, isDirectory, &rc_calf);
           if(rc_calf != SUCCESS){
               printError("Error in create and link file");
               rc_t = rc_calf;
           }
        }
    } else {
        printError("Error allocating pathName: insertDataIntoTree");
        rc_t = rc_ca;
    }
    *msg = rc_t;
    return parentNode;
}

//TODO
/**
 * IL MATTONE. 
 * NO BREAK IN THE BRICK
 */
TreeNode performInsert(char *path, char *completePath, TreeNode startingPoint, int isDirectory, int* msg){
    TreeNode parentNode = NULL;
    int match = 0;
    int found = 0;
    int counter = 0;
    int resetCounter = 0;
    int rc_idit = SUCCESS;
    //printf("PASTA startingPoint: ");
    //pasta(startingPoint);
    //printf("Ricevo -> path: %s, completePath: %s, isDirectory %d, msg %d\n", path, completePath, isDirectory, *msg);
    //printf("size: %d\n", startingPoint->children->size);
    TreeNode toExamine = NULL;
    TreeNode whereToInsert = NULL;
    if(startingPoint != NULL && path != NULL){
        //Decide where to insert
        //printf("Prima di decideWhereToAttach\n");
        toExamine = decideWhereToAttach(startingPoint, path, &counter, &found);
        printf("%d\n", toExamine);  //È MERDA DI NULL
        //printf("Dopo di decideWhereToAttach\n");
        /*whereToInsert = setWhereToInsert(startingPoint, toExamine, &found);
        //printf("Dopo di setWhereToInsert\n");
        if(found != -1){
            //printf("Non trovato\n");
            //printf("Prima di insertDataIntoTree\n");
            printf("Where to insert: %d\n", whereToInsert);
            parentNode = insertDataIntoTree(whereToInsert, path, completePath,  &counter, isDirectory, &rc_idit);
            //printf("Dopo di insertDataIntoTree\n");
            if(rc_idit != SUCCESS){
                printError("Error inserting data into tree in perform insert");
                *msg = rc_idit;
            }
        }*/
    } else {
        *msg = NULL_POINTER;
    }
    //Provo a salire il parent
    printf("Salgo il tree: \n");
    while(startingPoint != NULL){
        pasta(startingPoint);
        startingPoint = startingPoint->parent;
    }
    return parentNode;
}

/**
 * Function that schedule a file
 */
int scheduleFile(TreeNode toSchedule, Manager manager){
    int rc_t = SUCCESS;
    if(toSchedule != NULL){
        rc_t = checkAllocationError(manager->filesToAssign);
        if(rc_t == SUCCESS){
            rc_t = enqueue(manager->filesToAssign, (void *) toSchedule);
        } else {
            rc_t = MALLOC_FAILURE;
        }
    } else {
        rc_t = FILE_NULL_POINTER;
    }
    return rc_t;
}

/**
 * Function that assign file to manager
 */
int assignFileToManager(TreeNode toSchedule, List managers){
    int rc_t = SUCCESS;
    if(managers != NULL){
        Manager manager = (Manager) front(managers);
        rc_t = pop(managers);
        if(manager != NULL){
            if(rc_t == SUCCESS){
                scheduleFile(toSchedule, manager);
                rc_t = enqueue(managers, (void *) manager);
                if(rc_t != SUCCESS){
                    rc_t = ENQUEUE_MANAGER_ERROR;
                }
            } else {
                rc_t = UNEXPECTED_LIST_ERROR;
            }
        } else {
            rc_t = MANAGER_NULL_ERORR;
        }    
    } else {
        rc_t = NULL_POINTER;
    }
    return rc_t;
}

int insertAndSchedule(TreeNode currentDirectory, List managers, char *currentPath, char *completePath, int toSkip){
    int rc_t = SUCCESS;
    int rc_af = SUCCESS;
    TreeNode toSchedule = NULL;
    //LOCK 
    //pasta((void *)currentDirectory);
    toSchedule = performInsert(currentPath, completePath, currentDirectory, FILE, &rc_t);
    //printf("qui mi rompo\n");
    //MAYBE RELEASE THE LOCK???
    if(rc_t == SUCCESS){
        rc_af = assignFileToManager(toSchedule, managers);
        if(rc_af != SUCCESS){
            printError("Error in assign file to manager");
            rc_t = rc_af;
        }
        //HANDLE THE MULTIPLE ERROR WITH THE ERROR HANDLER    
    }
    return rc_t;
    //UNLOCK
}

int aborto(TreeNode currentDirectory, List managers, int *fd, char *currentPath, char *completePath, int toSkip){
    int bytesRead = 1;
    int rc_t = SUCCESS;
    int skipped = 0;
    char charRead;
    int counter = 0;
    int rc_af = SUCCESS;
    int rc_ia = SUCCESS;
    TreeNode toSchedule = NULL;
    while(bytesRead != 0 && rc_t == SUCCESS){
        bytesRead = readChar(fd[READ], &charRead);
        completePath[counter + skipped] = charRead;
        if(skipped >= toSkip){
            if(charRead == '\n' || charRead == '\0'){
                currentPath[counter] = '\0';
                completePath[counter + skipped] = '\0';
                //printf("Ciao ho path completo %s\n", completePath);
                if(counter > 0){
                    rc_ia = insertAndSchedule(currentDirectory, managers, currentPath, completePath, toSkip);
                    if(rc_ia != SUCCESS){
                        printError("Insert and schedule failed");
                    }
                }
                counter = 0;
                skipped = 0;
            }else{
                currentPath[counter] = charRead;
                counter++;
                //printf("Ho letto (1) %c che in ascii e' %d\n", charRead, charRead);
            }
        } else {
            skipped++;
        }  
    }
    return rc_t;
}

/**
 * Inizializza il currentPath: NB.
 * Questa roba deve essere modificata
 * Perform insert and assign file to manager
 */
int parseLineDescriptor(TreeNode currentDirectory, char *currentPath, int *fd, int toSkip, List managers){
    int rc_t = SUCCESS;
    int rc_af = SUCCESS;
    int rc_ab = SUCCESS;
    char *completePath = (char *)malloc(sizeof(char) * PATH_MAX);
    int rc_acp = checkAllocationError(completePath);
    if(rc_acp == SUCCESS){
        if(managers != NULL){
            rc_ab = aborto(currentDirectory, managers, fd, currentPath, completePath, toSkip);
            if(rc_ab != SUCCESS){
                rc_t = rc_ab;
                printError("Failed ABORTO function");
            }else{
                printList(managers, toStringManager);
            }
        } else {
            rc_t = NULL_POINTER;
        }
        free(completePath);
    }else{
        rc_t = rc_acp;
    }
    return rc_t;
}


/**
 * Function that read find result
 * It calls parse line descriptor that parse a line read from the descriptor and save files into the tree
 */
int saveFindResult(TreeNode currentDirectory, char *compactedPath,  char *currentPath, int toSkip, int isDirectory, List managers){
    int rc_t = SUCCESS;
    int fd[2];
    int rc_cds_1 = SUCCESS;
    int rc_cds_2 = SUCCESS;
    int rc_cdp = SUCCESS;
    int rc_pld = SUCCESS;
    int rc_exlp = SUCCESS;
    int rc_fd = createUnidirPipe(fd);
    if(rc_fd == SUCCESS){
        int f = fork();
        if (f > 0) {
            //TODO... Deal with pipe 
            rc_cds_1 = closeDescriptor(fd[WRITE]);
            //Chiama funzione mettere nuovo rc
            rc_pld = parseLineDescriptor(currentDirectory, currentPath, fd, toSkip, managers);
            //TODO... Deal with pipe
            rc_cds_2 = closeDescriptor(fd[READ]);
        } else if(f == 0) {
            //TODO... Deal with pipe
            rc_cds_1 = closeDescriptor(fd[READ]);
            rc_cdp = createDup(fd[WRITE], 1);
            rc_cds_2 = closeDescriptor(fd[WRITE]);
            rc_exlp = execlp("find", "find",  compactedPath, "-mindepth", "1", "-type", "f", NULL);
            //printf("fallisco miseramente\n");
        } else {
            printError("The program couldn't execute a fork");
            rc_t = FORK_FAILURE;
        }
    }else{
        printError("Pipe find in saveFindResult gone wrong");
    }
    //TODO Error handler e ritornare l'errore relativo, non quello totale
    if (rc_pld != SUCCESS){
        printError("Parse line descriptor gone wrong");
        rc_t = rc_pld;
    } else if (rc_cds_1 != SUCCESS){
        printError("Closing first descriptor gone wrong");
        rc_t = rc_cds_1;
    } else if (rc_cds_2 != SUCCESS){
        printError("Closing second descriptor gone wrong");
        rc_t = rc_cds_2;
    } else if (rc_cdp != SUCCESS){
        printError("Error creating dup");
        rc_t = rc_cdp;
    } else if (rc_exlp != SUCCESS){
        printError("Error executing exec");
        rc_t = rc_exlp;
    }
    return rc_t;
}

/**
 * Function that saves the files by reading what 
 */
int saveFiles(Tree fs, TreeNode currentDirectory, char *compactedPath, int toSkip, int isDirectory, List managers){
    int rc_t = SUCCESS;
    int rc_af = SUCCESS;
    int rc_ca = SUCCESS;
    int counter = 0;
    int skipped = 0;
    char *currentPath = malloc(PATH_MAX * sizeof(char));
    rc_ca = checkAllocationError(currentPath);
    TreeNode toSchedule = NULL;
    if(rc_ca == SUCCESS){
        if(isDirectory == DIRECTORY){
            rc_t = saveFindResult(currentDirectory, compactedPath, currentPath, toSkip, isDirectory, managers);
            if(rc_t != SUCCESS){
                printError("Error in save find result");
            }
        } else {
            while(compactedPath[skipped] != '\0'){
                if(skipped >= toSkip){
                    currentPath[counter] = compactedPath[skipped];
                    counter++;
                    skipped++;
                } else {
                    skipped++;
                }  
            }
            currentPath[counter] = '\0';
            //printf("nome file: %s\n", currentPath);
            //printf("path completo: %s\n", compactedPath);
            //LOCK
            toSchedule = performInsert(currentPath, compactedPath, currentDirectory, FILE, &rc_t);
            //MAYBE RELEASE THE LOCK???
            if(rc_t == SUCCESS){
                rc_af = assignFileToManager(toSchedule, managers);
                //HANDLE THE MULTIPLE ERROR WITH THE ERROR HANDLER
                if(rc_af != SUCCESS){
                    printError("Error in assign file to manager");
                    rc_t = rc_af;
                }     
            }
        }
        free(currentPath);
    } else {
        printError("Allocation error in save files");
        rc_t = rc_ca;
    }
    return rc_t;
}

/**
 * Function that read a line from bash call into the pipe opened
 */
int readBashLines(int *fd, char *dst){
    int rc_t = SUCCESS;
    int rc_cds_1;
    int rc_cds_2;
    int bytesRead;
    char charRead;
    int counter = 0;
    rc_cds_1 = closeDescriptor(fd[WRITE]);
    //Ho messo il dowhile
    do{
        bytesRead = readChar(fd[READ], &charRead);
        if(bytesRead > 0){
            if(charRead != '\n'){
                dst[counter] = charRead;
                counter++;
            }
        }
    }while(bytesRead > 0);

    if(bytesRead == -1){
        rc_t = READ_FAILURE;
    }
    dst[counter] = '\0';
    //TODO... Deal with pipe
    rc_cds_2 = closeDescriptor(fd[READ]);
    //Missing error handling
    if(rc_cds_1 != SUCCESS){
        printError("Allllllllllora, non funziona la close descriptor in fd write in read bash line");
    }else if(rc_cds_2 != SUCCESS){
        printError("Allllllllllora, non funziona la close descriptor in fd read in read bash line");
    }
    return rc_t;
}

/**
 * Function that call bash process and write on the pipe
 * It uses dup2 to override it's stdout
 */
int writeBashProcess(int *fd, char *argv[]){
    int rc_t;
    int rc_cds_1 = closeDescriptor(fd[READ]);
    int rc_cdp = createDup(fd[WRITE], 1);
    int rc_cds_2 = closeDescriptor(fd[WRITE]);
    int rc_ex = execvp(argv[0], argv);
    if(rc_ex < 0){
        //ExecCVP failure
        char *msgErr = (char *)malloc(300);
        int rc_ca = checkAllocationError(msgErr);
        if (rc_ca < 0) {
            //TODO... HARAKIRI PROBABLY
            printError("I can't allocate memory");
        } else {
            sprintf(msgErr, "Error while doing exec %s from bashCall\n", argv[0]);
            perror(msgErr);
            free(msgErr);
        }
    }
    //TODO Error Handling
    if(rc_cds_1 != SUCCESS){
        printError("Error closing descriptor read in write bash process");
    }else if(rc_cdp != SUCCESS){
        printError("Error create the dup in write bash process");
    }else if(rc_cds_2 != SUCCESS){
        printError("Error closing descriptor write in write bash process");
    }else if(rc_ex != SUCCESS){
        printError("Error exec in write bash process");
    }
    return rc_t;
}

/**
 * Function that call a bash process and pass to it the argv parameter
 * In case of success returns in dst what the bash prints
 */
int callBashProcess(char *dst, char *argv[]){
    int rc_t = SUCCESS;
    int rc_rb;
    int rc_wb;
    int fd[2];
    int rc_fd = createUnidirPipe(fd);
    if(rc_fd == SUCCESS){
        int f = fork();
        if (f > 0) {
            rc_rb = readBashLines(fd, dst);
        } else if (f == 0) {
            rc_wb = writeBashProcess(fd, argv);
        } else {
            printError("The program couldn't execute a fork");
            rc_t = FORK_FAILURE;
        }
        if(rc_t != SUCCESS ){
            if(rc_rb != SUCCESS){
                printError("Error in call bash process -> read");
            }else if(rc_wb != SUCCESS){
                printError("Error in call bash process -> write");
            }
        }
    }else{
        printError("Pipe creation in call bash process gone wrong");
    }
    
    return rc_t;
}

/**
 * Function that check the file type by calling a bash process
 */
int checkFileType(char *name){
    int rc_t = SUCCESS;
    int rc_sp = SUCCESS;
    int rc_ss = SUCCESS;
    char *command = (char *) malloc(sizeof(char)*(PATH_MAX*2 + 150));
    rc_t = checkAllocationError(command);
    if (rc_t < 0) {
        //TODO... HARAKIRI PROBABLY
        printError("I can't allocate memory");
    } else {
        rc_sp = sprintf(command, "([ -f '%s' ] && echo '0') || ([ -d '%s' ] && echo '1') || echo '2'", name, name);
        if(rc_sp > 0){
            char *argv[4] = {"sh", "-c", command,  NULL};
            char type[2];
            rc_t = callBashProcess(type, argv);
            if(rc_t == SUCCESS){
                rc_ss = sscanf(type, "%d", &rc_t);
                if(rc_ss <= 0){
                    printError("Error in checkFileType due to scanf, is it really able to fail?");
                    //Scegliere cosa fare, se rimetdiabile o meno, blocchiamo tutto o non permettiamo di inserire il dato all'interno del comando?
                }
            }
        }else{
            //Stessa cosa di sopra
            printError("Error in checkFileType due to sprintf, is it really able to fail?");
        }
        free(command);
    }
    //Handle error
    return rc_t;
}

/**
 * Function that compact the path given from input by calling a bash function
 */
int compactPath(char *string, char *cwd, int isAbsolute){
    int rc_t = SUCCESS;
    if(isAbsolute == ABSOLUTE){
        char *argv[4] = {"realpath", "-m", string,  NULL};
        rc_t = callBashProcess(string, argv);
    } else {
        char *argv[7] = {"realpath", "-m", "--relative-to", cwd, string,  NULL};
        rc_t = callBashProcess(string, argv);
    }
    return rc_t;
}

/**
 * Function in precompute path that goes up
 */
TreeNode goUp(Tree fs, TreeNode currentDirectory, char *path, int *toSkip){
    TreeNode startingNode = NULL;
    int keepGoing = 0;
    int counter = 3;
    startingNode = currentDirectory->parent;
    printf("La working directory e' %s, quella di partenza e' %s \n", ((FileInfo)startingNode->data)->name, ((FileInfo)currentDirectory->data)->name);
    while(path[counter] == '.' && keepGoing == 0){ //I still need to check if I need to go up more
        if(path[counter+1] == '.'){
            if(path[counter+2] == '/'){
                counter += 3;
                startingNode = startingNode->parent;
                if(startingNode == NULL){
                    keepGoing = -1;
                    printError("Something went terribly wrong in the path analysis so we'll attach it to the root");
                    startingNode = getRoot(fs);
                }
            }
        } else { //I'm arrived at the rigth level
             keepGoing = -1;
        }
    }    
    *toSkip = counter;
    printf("La working directory e' %s\n", ((FileInfo)startingNode->data)->name);
    return startingNode;
}

/**
 * Function that precomputes the starting directory in wich files will be addes
 */
TreeNode precomputeStartingDirectory(Tree fs, TreeNode currentDirectory, char *path, int *toSkip){
    TreeNode startingNode = NULL;
    *toSkip = 0;
    //printf("path: %s\n", path);
    if(path != NULL && fs != NULL && currentDirectory != NULL){ //Verify if everything is ok
        if(path[0] == '/'){ //if my path start with / then i'm talking about an Absolute Path
            startingNode = getRoot(fs);
            *toSkip = 1;
        } else if(path[0] == '.'){ //if my path starts with a dot...
            if(path[1] == '.'){ //and is followed by a dot...
                if(path[2] == '/'){ //then I need to go up but...
                    startingNode = goUp(fs, currentDirectory, path, toSkip);
                } else {// I didn't even know it could be possible to create a folder called .......something
                    printInfo("You created a monster of a folder but I still handled the Error");
                    startingNode = currentDirectory;
                }
            } else {
                if(path[1] == '/'){ //If the first dot is followed by a slash then I'm talking about the folder I'm in
                    *toSkip = 2;
                } //Otherwise I'm talking about an hidden folder
                startingNode = currentDirectory;
            }
        } else {//In any other case I'm talking about a folder or a file inside my current folder
            startingNode = currentDirectory;
        }
    }
    printf("devo skippare %d carattere/i e la working directory e' %s\n", *toSkip, ((FileInfo)startingNode->data)->name);
    return startingNode;
}

/**
 * Add Forward Slash if it is not empty
 */
void addForwardSlash(char *path){
    int stringLength = strlen(path);
    if (path[stringLength - 1] != '/'){
        path[stringLength] = '/';
        path[stringLength + 1] = '\0';
    }
}

/**
 * Check if a smth is a directory, a file or neither of them
 * If evertything goes well it precompute the starting directory and saves the files: add them in the Tree data structure
 */
int checkAnalyzeAndAssign(Tree fs, TreeNode currentDirectory, char *path, int *toSkip, List managers){
    int rc_t = SUCCESS;
    int rc_sf;
    int rc_cf = checkFileType(path);
    //printf("Sono cft e torno: %d\n", rc_cf);
    if (rc_cf == DIRECTORY){
        addForwardSlash(path);
    }
    if (rc_cf == DIRECTORY || rc_cf == FILE){
        TreeNode workingDirectory = precomputeStartingDirectory(fs, currentDirectory, path, toSkip);
        if(workingDirectory != NULL){
            //printf("path prima del save file: %s\n", path);
            rc_sf = saveFiles(fs, workingDirectory, path, *toSkip, rc_cf, managers);
            if(rc_sf != SUCCESS){
                printError("Failed to save files");
            }
        }else{
            rc_t = STARTING_DIRECTORY_FAILURE;
            printError("Failed to obtain the starting node for the inserts");
        }
    } else {
        if (rc_cf == NOT_EXISTING){
            char *msgInfo = (char *)malloc(sizeof(char) * (PATH_MAX + 300));
            int rc_ca = checkAllocationError(msgInfo);
            if (rc_ca < 0){
                //TODO... HARAKIRI PROBABLY
                printError("I can't allocate memory");
            } else {
                sprintf(msgInfo, "The File %s doesn't exist", path);
                printInfo(msgInfo);
                free(msgInfo);
            }
        }
    }
    //Handle error
    if(rc_t != SUCCESS){

    }else if(rc_sf != SUCCESS){
        rc_t = rc_sf;
    }
    return rc_t;
}
 

/**
 * Function that read the files and perform the 
 */
int doWhatAnalyzerDoes(char *cwd, Tree fs, TreeNode currentDirectory, List managers){
    char *path = (char *) malloc(sizeof(char)*PATH_MAX);
    int rc_t = SUCCESS;
    int rc_rd = SUCCESS;
    int rc_cf = SUCCESS;
    int rc_of = SUCCESS;
    int rc_cp = SUCCESS;
    int rc_caa = SUCCESS;
    int toSkip = 0;
    int isAbsolutePath = 0;
    int bytesRead = RELATIVE;
    //TODO... if path is too long the pipe will be dirty at the end (maybe with a while to read)
    rc_rd = readDescriptor(0, path, PATH_MAX);
    //printf("Ho letto %s, di lunghezza %d\n", path, (int)strlen(path));
    path[strlen(path) - 1] = '\0'; //FIX FOR THE MOMENT TO AVOID \n FROM INPUT
    //printf("path: %s, di lunghezza %d\n", path,(int) strlen(path));
    if (rc_rd <= 0){
        //TODO... Error During Reading: try next time maybe will be better
    } else if(path[0] == 0) {
        printInfo("Nothing to add because the string is empty");
    } else {
        if(path[0] == '/') isAbsolutePath = ABSOLUTE;
        //Precompute the TreeNode pointer from which the search will start
        rc_cp = compactPath(path, cwd, isAbsolutePath);
        if (rc_cp == SUCCESS){
           rc_caa = checkAnalyzeAndAssign(fs, currentDirectory, path, &toSkip, managers);
           if(rc_caa != SUCCESS){
               printError("Error in check analyze and assign, i dunno what it happened lmao");
           }
        } else {
            char *msgErr = (char *)malloc(300);
            int rc_ca = checkAllocationError(msgErr);
            if (rc_ca < 0){
                //TODO... HARAKIRI PROBABLY
                printError("I can't allocate memory");
            } else {
                sprintf(msgErr, "inisde analyzer compact path with pid: %d", getpid());
                printInfo(msgErr);
                free(msgErr);
            }
        }
    }
    printf("sizeeeeee %d\n", getRoot(fs)->children->size);
    free(path);
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

int parentInitExecPipe(const int toParent[], const int toChild[]) {
  int rc_t = 0;
  int rc_cl = closeDescriptor(toParent[WRITE_CHANNEL]);
  int rc_cl2 = closeDescriptor(toChild[READ_CHANNEL]);

  if (rc_cl == -1 || rc_cl2 == -1) {
    rc_t = PIPE_FAILURE;
  }
  return rc_t;
}

Manager newManager(){
    Manager manager = (Manager) malloc(sizeof(struct Manager));
    int rc_ca = checkAllocationError(manager);
    if(rc_ca == SUCCESS){
        manager->filesToAssign = newList();
        manager->filesInExecution = newList();
        if(manager->filesToAssign == NULL || manager->filesInExecution == NULL){
            free(manager->filesToAssign);
            free(manager->filesInExecution);
            free(manager);
        } else {
            manager->pipe = (int *) malloc(sizeof(int)*2);
            rc_ca = checkAllocationError(manager->pipe);
            if(rc_ca != SUCCESS){
                free(manager->filesToAssign);
                free(manager->filesInExecution);
                free(manager);
            }
        }
    }
    return manager;
}

int addManagers(List managers, int amount) {
  int rc_t = OK;
  int rc_en = OK;
  int i = 0;
  for (i = 0; i < amount && rc_t == OK; i++) {
    //TODO... create manager struct
    Manager manager = newManager();
    if (manager == NULL)
      rc_t = NEW_MANAGER_FAILURE;
    else {
      rc_en = push(managers, manager);
      if (rc_en == -1)
        rc_t = MALLOC_FAILURE;
      else {
        int toParent[2];
        int toChild[2];
        //TODO create pipe
        createUnidirPipe(toParent);
        createUnidirPipe(toChild);
        int managerPid = fork();
        if (managerPid > 0) {
          int rc_pp = parentInitExecPipe(toParent, toChild);
          if (rc_pp == -1)
            rc_t = PIPE_FAILURE;
          else {
            manager->m_pid = managerPid;
            manager->pipe[0] = toParent[READ_CHANNEL];
            manager->pipe[1] = toChild[WRITE_CHANNEL];
          }
        } else {
          managerInitPipe(toParent, toChild);
          execlp("./manager", "./manager", NULL);
        }
      }
    }
  }

  return rc_t;
}

int changeManagersAmount(List managers, const int currentManagers,
                        const int newManagers, List tables, List todo,
                        List doing, List done) {
  int rc_t;
  int delta;
  if (currentManagers > 0)
    delta = newManagers - currentManagers;
  else
    delta = newManagers;

  if (delta > 0) {
    rc_t = addManagers(managers, delta);
  } else {
    //rc_t = removeWorkers(managers, -delta, tables, todo, doing, done);
  }

  return rc_t;
}
/*
//FUNZIONE ART ATTACK
//FUNZIONE ART ATTACK
//FUNZIONE ART ATTACK
void *readDirectives(void *ptr) {
  sharedResources_t *sharedRes = (sharedResources_t *)(ptr);
  // TODO choose max length for path
  // TODO ATTENZIONE mettere i byte terminatori nel manager
  int rc_t = OK;
  char readBuffer[2] = "a";
  char *newPath = malloc(PATH_MAX * sizeof(char));
  int rc_al = checkAllocationError(newPath);
  if (rc_al < OK) {
    rc_t = MALLOC_FAILURE;
  }
  char nWorker[MAXLEN];
  int counter = 0;

  while (rc_t == OK) {
    counter = 0;
    do {
      int rc = readChar(READ_CHANNEL, readBuffer);
      newPath[counter++] = readBuffer[0];
    } while (readBuffer[0] != '\0' && readBuffer[0] != '\n');
    newPath[counter] = '\0';
    if (newPath[strlen(newPath) - 1] == '\n') {
      newPath[strlen(newPath) - 1] = '\0';
    }

    counter = 0;
    do {
      int rc = readChar(READ_CHANNEL, readBuffer);
      nWorker[counter++] = readBuffer[0];
    } while (readBuffer[0] != '\0' && readBuffer[0] != '\n');
    nWorker[counter] = '\0';

    pthread_mutex_lock(&(sharedRes->mutex));
    int rc_sc = sscanf(nWorker, "%d", &(sharedRes->directive->newNWorker));
    if (rc_sc == 0 ||
        (sharedRes->directive->newNWorker == 9 && strcmp(nWorker, "9") != 0)) {
      rc_t = CAST_FAILURE;
    }
    strcpy(sharedRes->directive->path, newPath);

    if (newPath[0] == '\0' || nWorker[0] == '\0') {
      char *msgErr = (char *)malloc(300);
      int rc_ca = checkAllocationError(msgErr);
      if (rc_ca < 0) {
        printError("I can't allocate memory");
      } else {
        sprintf(msgErr, "inisde worker with pid: %d", getpid());
        printError(msgErr);
        free(msgErr);
      }
    } else if (strcmp(sharedRes->directive->lastPath,
                      sharedRes->directive->path) == 0 &&
               sharedRes->directive->newNWorker ==
                   sharedRes->directive->currentWorkers) {
      sharedRes->directive->directiveStatus = SUMMARY;
    } else if (rc_t == OK) {
      sharedRes->directive->directiveStatus = NEW_DIRECTIVES;
    }
    pthread_mutex_unlock(&(sharedRes->mutex));

    // TODO fix sleep time
    usleep(500000);
  }
  free(newPath);
  // return rc_t;
}*/

int sendFile(Manager manager, TreeNode file) {
  int rc_t = OK;
  int rc_po = pop(manager->filesToAssign);
  int rc_pu = enqueue(manager->filesInExecution, file);
  if (rc_po != -1 && rc_pu != -1) {
    int *pipe = manager->pipe;
    char *path = malloc(PATH_MAX * sizeof(char));
    int rc_al = checkAllocationError(path);
    if (rc_al < OK)
      rc_t = MALLOC_FAILURE;
    else {
      // TODO add check for sprintf (wrapping function)
      sprintf(path, "%s", ((FileInfo)file->data)->path);
      //TODO write m in pipe to manager
      int rc_wr = writeDescriptor(pipe[WRITE_CHANNEL], path);

      if (rc_wr < OK)
        rc_t = SEND_FAILURE;

      free(path);
    }
  } else
    rc_t = ASSIGNFILE_MEMORY_FAILURE;

  return rc_t;
}

int isManagerAlive(Manager m) {
  int rc_t = OK;
  int returnCode = kill(m->m_pid, 0);
  if (returnCode != 0) {
    rc_t = DEAD_PROCESS;
  }
  return rc_t;
}
/*
void destroyManager(void *data) {
  Manager manager = (Manager) data;

  if (manager->pipe != NULL) {
    closeDescriptor(manager->pipe[0]);
    closeDescriptor(manager->pipe[1]);
  }
  if (manager-> != NULL) {
    destroyWorker(worker->doing);
  }
  free(worker->pipe);
  free(worker->table);
  free(worker->doing);
  free(worker);
}

int rescheduleFiles(List toDestroy, List toReschedule){
    int rc_t =  SUCCESS;
    if(isEmptyList(toReschedule) == EMPTY){
        if(isEmptyList(toDestroy) != EMPTY){
            rc_t = swap(toDestroy, toReschedule);
        }
    } else {
        TreeNode file = NULL;
        while(toDestroy == NOT_EMPTY){
            file = (TreeNode) front(toDestroy);
            rc_t = pop(toDestroy);
            if(rc_t == SUCCESS){

            }
        }
    }
    return rc_t;
}

int manageFileToSend(List managers) {
  int rc_t = OK;
  int rc_po = OK;
  int rc_pu = OK;
  int rc_ww = OK;
  int managerSize = managers->size;
  int isManagerAlive = OK;

  int i = 0;
  for(i = 0; i < managerSize; i++) {
    Manager m;
    m = (Manager) front(managers);
    isManagerAlive = isAlive(m);
    rc_po = pop(managers);
    if (m != NULL) {
      if (isManagerAlive == OK) {
        rc_pu = enqueue(managers, m);
        if (rc_po == -1 && rc_pu == -1)
            rc_t = NEW_MANAGER_FAILURE;
        else {
            if (isEmptyList(m->filesToAssign) == EMPTY) {
                //rc_ww = getWorkerWork(w, tables, todo, doing, done);
                if (rc_ww)
                rc_t = WORK_FAILURE;
            } else {
                TreeNode file = front(m->filesToAssign);
                if (file != NULL) {
                rc_t = sendFile(managers, file);
                } else {
                rc_t = ASSIGNWORK_FAILURE;
                }
                
            }
        }
      } else {
          destroyManager(m, managers);
          rc_t = DEAD_PROCESS;
      }
    }
  }

  return rc_t;
}
*///
////
Tree fs = NULL;
//
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
/**
 * Signal handler for debug
 */
void sighandle_int(int sig){
    destroyTree(fs);
    signal(SIGINT, SIG_DFL);
}


/**
 * Function that init the fileInfo root of the tree
 */
//Ho spostato la funzione, speriamo bene che tutto funzioni correttametne
FileInfo initFileInfoRoot(int *msg){
    FileInfo rootData;
    char *rootName = (char *)malloc(sizeof(char)*2);
    int rc_ca = checkAllocationError(rootName);
    int rc_nc = SUCCESS;
    if (rc_ca < 0) {
        //TODO... HARAKIRI PROBABLY
        printError("I can't allocate memory");
    } else {
        if (strcpy(rootName, "/") != NULL)
            rootData = newFileInfo((void *)rootName, DIRECTORY, NULL, &rc_nc);
    }
    //Error checking
    if(rc_ca == MALLOC_FAILURE || rc_nc == MALLOC_FAILURE){
        *msg = MALLOC_FAILURE;
    }
    return rootData;
}

/**
 * Function that init the analyzer tree 
 */
Tree initializeAnalyzerTree(FileInfo data, int *msg, void destroyer(void *)){
    Tree tree = NULL;
    tree = newTree((void *)data, msg, destroyer, NULL);
    if(*msg != SUCCESS){
        char *msgErr = (char *)malloc(300);
        int rc_ca = checkAllocationError(msgErr);
        if (rc_ca < 0) {
            //TODO... HARAKIRI PROBABLY
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
int getCwd(char *dst){
    int rc_cwd = SUCCESS;
    if(getcwd(dst, PATH_MAX) == NULL){
        rc_cwd = CWD_FAILURE;
    }
    return rc_cwd;
}

int main(){
    signal(SIGCHLD, SIG_IGN);
    signal(SIGINT, sighandle_int);
    //Tree fs = NULL;
    int currentManagers = 3;
    TreeNode currentDirectory = NULL;
    FileInfo root = NULL;
    //TODO check for literals, ehm... I mean errors!
    List managers = newList();
    addManagers(managers, currentManagers);
    char cwd[PATH_MAX];
    int rc_cwd;
    int rc_ir;
    int rc_it;
    int rc_pi;
    int rc_t = SUCCESS;
    rc_cwd = getCwd(cwd);
    //Alla fine tutto  printError verranno sostituiti con un handler dell'errore, quindi si, sono brutti, ma diciamo che è rimediabile
    if(rc_cwd == SUCCESS){
        //CANNOT MOVE THIS CODE (AT LEAST IF YOU DON'T WANT TO DEAL WITH COPY OF POINTER)
        root = initFileInfoRoot(&rc_ir);
        if(rc_ir == SUCCESS){
            //Inizializzazione dell'albero
            fs = initializeAnalyzerTree(root, &rc_it, destroyFileInfo);
            if(rc_it == SUCCESS){
                currentDirectory = performInsert(cwd, NULL, getRoot(fs), DIRECTORY, &rc_pi);
                if(rc_pi != SUCCESS){
                    printError("Unable to insert the current directory :(");
                }
            }else{
                printError("I am not able to initialize the root of the tree, that's terrible guys");
            }
        }else{
            printError("I am not able to allocate the root of the tree, this is very bad");
        }
        while(rc_t == SUCCESS){
            rc_t = doWhatAnalyzerDoes(cwd, fs, currentDirectory, managers);
        }
        destroyTree(fs);
    }else{
        printError("I am not able to find the current working directory. This is a disgrace");
    }
    return rc_t;
}
