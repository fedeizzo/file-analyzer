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
#define REMOVE_MANAGER_FAILURE -26

#define INT_MAX_LEN 12
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
    printf("free destroyFileInfo\n");
    free(data->name);
    free(data->path);
    free(data->fileTable);
    free(toDestroy);
    printf("dopo destroyFileInfo\n");
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
    if(*msg == SUCCESS){
        toRtn->name = name;
        toRtn->isDirectory = isDirectory;
        toRtn->fileTable = (int *) calloc(NCHAR_TABLE, sizeof(int));
        *msg = checkAllocationError(toRtn->fileTable);
        if(*msg == SUCCESS){
            *msg = allocatePath(toRtn, path);
            if(*msg != SUCCESS){
                printf("free file table\n");
                free(toRtn->fileTable);
                printf("free toRtn\n");
                free(toRtn);
                printf("dopo free toRtn\n");
            }
        } else {
            printf("free toRtn 2\n");
            free(toRtn);
            printf("dopo free toRtn 2\n");
        }
        
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
    printf("size: %d\n", manager->filesToAssign->size);
    //printList(manager->filesToAssign, pasta);
}

Node handleMath(TreeNode toExamine, FileInfo dataToExamine, char *path, int *found, int *match, int *counter, int *resetCounter, int *tmpCounter){
    Node actualNode = NULL;
    if(*match == 0){
        if(dataToExamine->name[*tmpCounter] == '\0'){ //Two names matches perfectly
            //printf("MATCH\n");
            if(path[*counter] == '\0'){ //It's already a folder or a file in the actual tree no need to insert again
                *found = -1;
                //printf("File Already Inserted\n");
            } else {
                //printf("%s è l'uomo che fa per me\n", dataToExamine->name);
                if(isEmptyList(toExamine->children) == NOT_EMPTY){ //If I have some inner files then we need to check if we can continue to have a match
                    actualNode = toExamine->children->head;
                } else { //I've arrived at the deepest known part in the tree which has already been inserted in the tree
                    *found = 1;
                }
                *resetCounter = *counter + 1;
                *counter = *resetCounter;
            }
        } else { //The children name contains the name in the path but they're different
            *counter = *resetCounter;
            *match = -1;
            //printf("I'm a false positive\n");
        }
    }
    return actualNode;
}

int checkIfMatch(FileInfo dataToExamine, char *path, int *counter, int *resetCounter, int *tmpCounter){
    int match = 0;
    while(path[*counter] != '/' && path[*counter] != '\0' && match == 0){
        if(path[*counter] == dataToExamine->name[*tmpCounter]){
            *counter += 1;
            *tmpCounter += 1;
            //printf("counter = %d, tmpCounter = %d\n", *counter, *tmpCounter);
            //printf("\tmatch carattere\n");
        } else {
            //printf("\tnon faccio match per colpa di %c che dovrebbe essere %c\n", dataToExamine->name[*tmpCounter], path[*counter]);
            *counter = *resetCounter;
            match = -1;
        }
    }
    //printf("counter = %d, resetCounter = %d, match = %d\n", *counter, *resetCounter, match);
    return match;
}

Node getNodeWhereToInsert(TreeNode toExamine, FileInfo dataToExamine, Node actualNode, char *path, int *found, int *match, int *counter, int *resetCounter, int *tmpCounter){
    if(dataToExamine != NULL){
        if(dataToExamine->name != NULL){
            //printf("counter %d\n", counter);
            //printf("controllo %s, tmpCounter = %d\n", dataToExamine->name, *tmpCounter);
            *match = checkIfMatch(dataToExamine, path, counter, resetCounter, tmpCounter);
            Node tmp = handleMath(toExamine, dataToExamine, path, found, match, counter, resetCounter, tmpCounter);
            if(tmp != NULL){
                actualNode = tmp;
            } else { //Case no match
                actualNode = actualNode->next;
            }
        }
    }
    return actualNode;
}

TreeNode createNewTreeElement(FileInfo dataToInsert, TreeNode toInsert, TreeNode whereToInsert, int *tmpCounter, char *pathName, int isDirectory, char * completePath, int *msg){
    int rc_nc = SUCCESS;
    int rc_tc = SUCCESS;
    pathName[*tmpCounter] = '\0';
    //printf("file: %s\n", pathName);
    dataToInsert = newFileInfo(pathName, isDirectory, completePath, &rc_nc);
    toInsert = newTreeNode(whereToInsert, (void *) dataToInsert, &rc_tc);
    if(rc_nc == SUCCESS && rc_tc == SUCCESS){
        linkChild(whereToInsert, toInsert);
    } else {
        printf("free createNewTreeElement\n");
        free(dataToInsert);
        free(toInsert);
        printf("dopo free createNewTreeElement\n");
        *msg = MALLOC_FAILURE;
    }
    return toInsert;
}

TreeNode createNewTwig(TreeNode whereToInsert, int *counter, char *path, char *pathName, int isDirectory, char * completePath, int *msg){
    TreeNode toRtn = NULL;
    TreeNode toInsert = NULL;
    FileInfo dataToInsert = NULL;
    int tmpCounter = 0;
    while(path[*counter]!='\0' && *msg == SUCCESS){
        //printf("giro nel ciclo\n");
        if(path[*counter] == '/'){
            if(*counter != 0){ //Check if it's the root
                whereToInsert = createNewTreeElement(dataToInsert, toInsert, whereToInsert, &tmpCounter, pathName, DIRECTORY, NULL, msg);
                tmpCounter = 0;
                if(*msg == SUCCESS){
                    pathName = (char *)malloc(sizeof(char)*PATH_MAX);
                    *msg = checkAllocationError(pathName);
                }
            }
        } else {
            pathName[tmpCounter] = path[*counter];
            tmpCounter += 1;
        }
        *counter += 1;
    }
    if(*msg == SUCCESS){
        toRtn = createNewTreeElement(dataToInsert, toInsert, whereToInsert, &tmpCounter, pathName, isDirectory, completePath, msg);
    }
    return toRtn;
}

/**
 * Perform insert
 */
TreeNode performInsert(char *path, char *completePath, TreeNode startingPoint, int isDirectory, int* msg){
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
    //printf("Ricevo -> path: %s, completePath: %s, isDirectory %d, msg %d\n", path, completePath, isDirectory, *msg);
    //printf("size: %d\n", startingPoint->children->size);
    if(startingPoint != NULL && path != NULL){
        if(isEmptyList(startingPoint->children) == NOT_EMPTY){
            actualNode = startingPoint->children->head;
            while(found == 0 && actualNode!=NULL){
                match = 0;
                toExamine = (TreeNode) actualNode->data;
                if(toExamine != NULL){
                    dataToExamine = (FileInfo) toExamine->data;
                    actualNode = getNodeWhereToInsert(toExamine, dataToExamine, actualNode, path, &found, &match, &counter, &resetCounter, &tmpCounter);
                }
                tmpCounter = 0;
            }
        }
        if(found == 0){ //The new Element wasn't a children of mine then i need to add it to my children
            if(toExamine == NULL){ //I need to attach the children to the Starting Point
                whereToInsert = startingPoint;
            } else { //The new node is my brother so I need to attach the children to my father
                whereToInsert = toExamine->parent;
            }
        } else if (found == 1){ //The element it's my first child (Probably never going to happen buuuuuut... YNK)
            whereToInsert = toExamine;
        }
        if(found != -1){
            //printf("sono il padre del nodo che stai analizzando e sono %s\n", ((FileInfo)whereToInsert->data)->name);
            //printf("size: %d\n", whereToInsert->children->size);
            //EFFICENCY OVER THE MEMORY OVER NINE THOUSAND
            char* pathName = (char *)malloc(sizeof(char)*PATH_MAX);
            //tmpCounter = 0;
            rc_ca = checkAllocationError(pathName);
            if(rc_ca == SUCCESS){
                toRtn = createNewTwig(whereToInsert, &counter, path, pathName, isDirectory, completePath, &rc_ca);
            }
        }
        *msg = rc_ca;
    } else {
        *msg = NULL_POINTER;
    }
    return toRtn;
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
                sleep(5);
                manageFileToSend(managers, 4);
            }
        } else {
            rc_t = NULL_POINTER;
        }
        printf("free complete path\n");
        free(completePath);
        printf("dopo free compete path\n");
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
        printf("free currentPath\n");
        free(currentPath);
        printf("dopo free currentPath\n");
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
        printf("free command\n");
        free(command);
        printf("dopo free command");
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
        //printf("sono la current directory %s\n", ((FileInfo)currentDirectory->data)->name);
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
int doWhatAnalyzerDoes(char *cwd, Tree fs, TreeNode currentDirectory, List managers, int currentManagers){
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
    printf("free path\n");
    free(path);
    printf("dopo free path\n");
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
            printf("free newManager\n");
            free(manager->filesToAssign);
            free(manager->filesInExecution);
            free(manager);
            printf("dopo free newManager\n");
        } else {
            manager->pipe = (int *) malloc(sizeof(int)*2);
            rc_ca = checkAllocationError(manager->pipe);
            if(rc_ca != SUCCESS){
                printf("free newManager2\n");
                free(manager->filesToAssign);
                free(manager->filesInExecution);
                free(manager);
                printf("dopo free newManager2\n");
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

Manager newReplaceManager(List filesToAssign, List filesInExecution){
    Manager manager = (Manager) malloc(sizeof(struct Manager));
    int rc_ca = checkAllocationError(manager);
    if(rc_ca == SUCCESS){
        manager->filesInExecution = filesInExecution;
        manager->filesToAssign = filesToAssign;  
        manager->pipe = (int *) malloc(sizeof(int)*2);
        rc_ca = checkAllocationError(manager->pipe);
        if(rc_ca != SUCCESS){
            printf("free newReplaceElement\n");
            free(manager->filesToAssign);
            free(manager->filesInExecution);
            free(manager);
            printf("dopo free newReplaceManager\n");
        }
    }
    return manager;
}

int replaceManager(List managers, List filesToAssign, List filesInExecution) {
  int rc_t = OK;
  int rc_en = OK;
    Manager manager = newReplaceManager(filesToAssign, filesInExecution);
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

/**
 * Naive solution. It is not the best solution, but it is cheap
 * The function will assign 
 */
int endManager(Manager m, List managers){
    List toSchedule = newList();
    int rc_t = SUCCESS;
    int rc_eqs = SUCCESS;
    int rc_eqm = SUCCESS;
    if(toSchedule == NULL){
        swap(toSchedule, m->filesInExecution);
        concat(toSchedule, m->filesToAssign);
        //Scheduling circolare
        if(managers != NULL){
            while(isEmptyList(toSchedule) == NOT_EMPTY && rc_t == SUCCESS){
                Manager tmpManager = front(managers); 
                pop(managers);
                rc_eqs = enqueue(tmpManager->filesToAssign, front(toSchedule));
                pop(toSchedule);
                rc_eqm = enqueue(managers, tmpManager);
                if(rc_eqs == -1 || rc_eqm == -1){
                    rc_t = MALLOC_FAILURE;
                }
            }
        }
    }else{
        rc_t = MALLOC_FAILURE;
    }
    return rc_t;
}

void destroyManager(void *data) {
  Manager manager = (Manager) data;
  if (manager->pipe != NULL) {
    closeDescriptor(manager->pipe[0]);
    closeDescriptor(manager->pipe[1]);
  }
  printf("free destroyManager\n");
  free(manager->pipe);
  free(manager->filesInExecution);
  free(manager->filesToAssign);
  free(manager);
  printf("dopo free destroyManager\n");
}

int removeManagers(List managers, int amount) {
  int rc_t = OK;
  while(amount != 0 && managers->size != 0){
    Manager m = front(managers);
    if (m != NULL) {
      int rc_po = pop(managers);
      endManager(m, managers);
      kill(m->m_pid, SIGKILL);
      destroyManager((void *) m);
      if (rc_po == -1)
        rc_t = REMOVE_MANAGER_FAILURE;  //VEDERE COME GESTIRE
      amount--;
    } else {
      rc_t = REMOVE_MANAGER_FAILURE;    //VEDERE COME GESTIRE
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
    rc_t = removeManagers(managers, delta);
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

int sendFile(Manager manager, TreeNode file, int currentWorker) {
  int rc_t = OK;
  int rc_po = pop(manager->filesToAssign);
  int rc_en = enqueue(manager->filesInExecution, file);

  if (rc_po != -1 && rc_en != -1) {
    int *pipe = manager->pipe;
    char *path = malloc((PATH_MAX + 1)* sizeof(char));
    char *nworkers = malloc((INT_MAX_LEN + 1) * sizeof(char));
    int rc_al = checkAllocationError(path);
    int rc_al2 = checkAllocationError(nworkers);
    if (rc_al < OK || rc_al2 < OK)
      rc_t = MALLOC_FAILURE;
    else {
      // TODO add check for sprintf (wrapping function)
      int rc_sp = sprintf(path, "%s", ((FileInfo)file->data)->path);
      path[strlen(path)] = '\n';
      int rc_sp1 = sprintf(nworkers, "%d", currentWorker);
      nworkers[strlen(nworkers)] = '\n';
      //TODO write m in pipe to manager
      if(rc_sp > 0 && rc_sp1 > 0){
          printf("sprintf ok\n");
          int rc_wr = writeDescriptor(pipe[WRITE_CHANNEL], path);
          int rc_wr_2 = writeDescriptor(pipe[WRITE_CHANNEL], nworkers);
          if (rc_wr < OK || rc_wr_2 < OK)
            rc_t = SEND_FAILURE;
      } else 
          rc_t = SEND_FAILURE;
      printf("free path\n");
      free(path);
      printf("free nworkers\n");
      free(nworkers);
      printf("dopo free nworkers\n");
    }
  } else
    rc_t = ASSIGNFILE_MEMORY_FAILURE;
  printf("rc_t: %d\n", rc_t);
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

int rescheduleFiles(List toDestroy, List toReschedule){
    int rc_t =  SUCCESS;
    if(isEmptyList(toReschedule) == EMPTY){
        if(isEmptyList(toDestroy) != EMPTY){
            rc_t = swap(toDestroy, toReschedule);
        }
    } else {
        TreeNode file = NULL;
        while(isEmptyList(toDestroy) == NOT_EMPTY && rc_t == SUCCESS){
            file = (TreeNode) front(toDestroy);
            rc_t = pop(toDestroy);
            if(rc_t == SUCCESS){
                rc_t = enqueue(toReschedule, (void*) file);
            }
        }
    }
    return rc_t;
}

int saveManagerWork(Manager m, List managers){
    int rc_t = SUCCESS;
    List saveToAssign = newList();
    List inExecution = newList();
    if(saveToAssign != NULL && inExecution != NULL){
        //Prevent the freeing of m lists, perform concatenation in O(1)
        swap(inExecution, m->filesInExecution);
        swap(saveToAssign, m->filesToAssign);
        rc_t = concat(saveToAssign, inExecution);
        if(rc_t == SUCCESS){
            rc_t = replaceManager(managers, saveToAssign, inExecution);
        }else{
            printError("Unable to concat the lists by preventing manager's works wasting\n");
        }
    }else{
        printError("Error creating lists in saveManagerWork \n");
        rc_t = MALLOC_FAILURE;
    }
    return rc_t;
}

int manageFileToSend(List managers, int currentWorker) {
  int rc_t = OK;
  int rc_po = OK;
  int rc_en = OK;
  int rc_ww = OK;
  int rc_smw = OK;
  int managerSize = managers->size;
  int isAliveM = OK;
  int i = 0;
  for(i = 0; i < managerSize; i++) {
    Manager m;
    m = (Manager) front(managers);
    isAliveM = isManagerAlive(m);
    rc_po = pop(managers);
    if (m != NULL) {
      if (isAliveM == OK) {
        rc_en = enqueue(managers, m);
        if (rc_po == -1 && rc_en == -1)
            rc_t = NEW_MANAGER_FAILURE;
        else {
            if (isEmptyList(m->filesToAssign) == EMPTY) {
                //TODO... Take data from managers
                //rc_ww = getWorkerWork(w, tables, todo, doing, done);
                if (rc_ww)
                rc_t = WORK_FAILURE;
            } else {
                TreeNode file = front(m->filesToAssign);
                if (file != NULL) {
                    rc_t = sendFile(m, file, currentWorker);
                } else {
                    rc_t = ASSIGNWORK_FAILURE;
                }
            }
        }
      } else {
          rc_smw = saveManagerWork(m, managers);
          //Error in save Manager Work
          if(rc_smw != OK){
            printError("Error in save Manager Work\n");
          }
          destroyManager((void *) m);
          rc_t = DEAD_PROCESS;
      }
    }
  }
  printf("esco da manage file to send\n");
  return rc_t;
}

Tree fs = NULL;

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
            printf("aspetto un input\n");
            rc_t = doWhatAnalyzerDoes(cwd, fs, currentDirectory, managers, currentManagers);
            printf("ho completato l'input\n");
        }
        destroyTree(fs);
    }else{
        printError("I am not able to find the current working directory. This is a disgrace");
    }
    destroyList(managers, destroyManager);
    return rc_t;
}