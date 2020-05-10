#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <limits.h>
#include "../tree/tree.h"

#define READ 0
#define WRITE 1

#define RELATIVE 0
#define ABSOLUTE 1

#define READ_FAILURE -3

#define DIRECTORY 1
#define FILE 0
#define NOT_EXISTING 2

#define FAILURE -1
#define SUCCESS 0

/**
 * Data structure that rapresent a file that can be a directory or not
 */
typedef struct FileInfo{
    char *name;
    int isDirectory;
} *FileInfo;

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
 * Costructor of the file info structure
 */
FileInfo newFileInfo(char *name, int isDirectory, int *msg){
    FileInfo toRtn = (FileInfo) malloc(sizeof(struct FileInfo));
    *msg = checkAllocationError(toRtn);
    if(*msg == SUCCESS){
        toRtn->name = name;
        toRtn->isDirectory = isDirectory;
    }
    return toRtn;
}

/**
 *  File info toString
 */
void toStringFileInfo(void *data){
    printf("File : %s isFolder %d\n", ((FileInfo)((TreeNode)data)->data)->name, ((FileInfo)((TreeNode)data)->data)->isDirectory);
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
TreeNode performInsert(char *path, TreeNode startingPoint, int isDirectory, int* msg){
    TreeNode toRtn = NULL;
    int rc_nc = SUCCESS;
    int rc_tc = SUCCESS;
    int rc_ca = SUCCESS;
    int found = 0;
    int match = 0;
    int counter = 0;
    int tmpCounter = 0;
    int resetCounter = 0;
    printf("%s\n", path);
    //printf("size: %d\n", startingPoint->children->size);
    Node actualNode = NULL;
    TreeNode toExamine = NULL;
    FileInfo dataToExamine = NULL;
    if(startingPoint != NULL && path != NULL){
        if(isEmptyList(startingPoint->children) == NOT_EMPTY){
            actualNode = startingPoint->children->head;
            while(found == 0 && actualNode!=NULL){
                //printf("size: %d\n", startingPoint->children->size);
                match = 0;
                toExamine = (TreeNode) actualNode->data;
                if(toExamine != NULL){
                    dataToExamine = (FileInfo) toExamine->data;
                    if(dataToExamine != NULL){
                        if(dataToExamine->name != NULL){
                            //printf("controllo %s, tmpCounter = %d\n", dataToExamine->name, tmpCounter);
                            while(path[counter] != '/' && path[counter] != '\0' && match == 0){
                                if(path[counter] == dataToExamine->name[tmpCounter]){
                                    counter++;
                                    tmpCounter++;
                                    //printf("\tmatch carattere\n");
                                } else {
                                    //printf("\tnon faccio match per colpa di %c che dovrebbe essere %c\n", dataToExamine->name[tmpCounter], path[counter]);
                                    counter = resetCounter;
                                    tmpCounter = 0;
                                    match = -1;
                                }
                            }
                            //printf("controllo (2) %s, tmpCounter = %d\n", dataToExamine->name, tmpCounter);
                            if(match == 0){
                                if(dataToExamine->name[tmpCounter] == '\0'){ //Two names matches perfectly
                                    //printf("MATCH\n");
                                    if(path[counter] == '\0'){ //It's already a folder or a file in the actual tree no need to insert again
                                        found = -1;
                                        //printf("File Already Inserted\n");
                                    } else {
                                        if(isEmptyList(toExamine->children) == NOT_EMPTY){ //If I have some inner files then we need to check if we can continue to have a match
                                            actualNode = toExamine->children->head;
                                        } else { //I've arrived at the deepest known part in the tree which has already been inserted in the tree
                                            found = 1;
                                        }
                                        resetCounter = counter + 1;
                                        counter = resetCounter;
                                        tmpCounter = 0;
                                    }
                                } else { //The children name contains the name in the path but they're different
                                    counter = resetCounter;
                                    tmpCounter = 0;
                                    match = -1;
                                    //printf("I'm a false positive\n");
                                }
                            }
                        }
                    }
                }
                if(match != 0){ //if I didn't match  then i need to look if a brother of mine has maybe the folder/file I'm looking for
                    actualNode = actualNode->next;
                }
            }
        }
        TreeNode whereToInsert = NULL;
        TreeNode toInsert = NULL;
        FileInfo dataToInsert = NULL;
        if(found == 0){ //The new Element wasn't a children of mine then i need to add it to my children
            if(toExamine == NULL){ //I need to attach the children to the Starting Point
                whereToInsert = startingPoint;
            } else { //The new node is my brother so I need to attach the children to my father
                whereToInsert = toExamine->parent;
            }
        } else if (found == 1){ //The element it's my first child (Probably never going to happen buuuuuut... YNK)
            whereToInsert = toExamine;
        }
        //printf("sono il padre del nodo che stai analizzando e sono %s\n", ((FileInfo)whereToInsert->data)->name);
        //printf("size: %d\n", whereToInsert->children->size);
        //EFFICENCY OVER THE MEMORY OVER NINE THOUSAND
        char* pathName = (char *)malloc(sizeof(char)*PATH_MAX);
        tmpCounter = 0;
        rc_ca = checkAllocationError(pathName);
        if(rc_ca == SUCCESS){
            while(path[counter]!='\0' && rc_ca == SUCCESS){
                if(path[counter] == '/'){
                    if(counter != 0){ //Check if it's the root
                        pathName[tmpCounter] = '\0';
                        dataToInsert = newFileInfo(pathName, 1, &rc_nc);
                        toInsert = newTreeNode(whereToInsert, (void *) dataToInsert, &rc_tc);
                        if(rc_nc == SUCCESS && rc_tc == SUCCESS){
                            linkChild(whereToInsert, toInsert);
                            pathName = (char *)malloc(sizeof(char)*PATH_MAX);
                            rc_ca = checkAllocationError(pathName);
                            tmpCounter = 0;
                            whereToInsert = toInsert;
                        } else {
                            free(dataToInsert);
                            free(toInsert);
                            rc_ca = FAILURE;
                        }
                    }
                } else {
                    pathName[tmpCounter] = path[counter];
                    tmpCounter++;
                }
                counter++;
            }
            if(rc_ca == SUCCESS){
                pathName[tmpCounter] = '\0';
                //printf("file: %s\n", pathName);
                dataToInsert = newFileInfo(pathName, isDirectory, &rc_nc);
                toInsert = newTreeNode(whereToInsert, (void *) dataToInsert, &rc_tc);
                if(rc_nc == SUCCESS && rc_tc == SUCCESS){
                    linkChild(whereToInsert, toInsert);
                } else {
                    free(dataToInsert);
                    free(toInsert);
                    //TODO... MALLOC FAILURE
                    rc_ca = FAILURE;
                }
                if(isDirectory){
                    toRtn = toInsert;
                }
            }
        }
        *msg = rc_ca;
    } else {
        *msg = NULL_POINTER;
    }
    return toRtn;
}

/**
 * Function that saves the files
 */
int saveFiles(Tree fs, TreeNode currentDirectory, char *dir, int toSkip, int isDirectory){
    int msg = SUCCESS;
    if(isDirectory == DIRECTORY){
        int rc_ca = SUCCESS;
        int counter = 0;
        char charRead = 0;
        int skipped = 0;
        int bytesRead = 1;
        char *currentPath = malloc(PATH_MAX * sizeof(char));
        rc_ca = checkAllocationError(currentPath);
        if(rc_ca == SUCCESS){
            int fd[2];
            pipe(fd);
            int f = fork();
            if (f > 0) {
                close(fd[WRITE]);
                while(bytesRead != 0 && msg == SUCCESS){
                    bytesRead = readChar(fd[READ], &charRead);
                        if(skipped >= toSkip){
                            if(charRead == '\n' || charRead == '\0'){
                                currentPath[counter] = '\0';
                                if(counter > 0){
                                    //printf("path: %s\n", currentPath);
                                    //printf("Ho letto (2) %c che in ascii e' %d\n", charRead, charRead);
                                    performInsert(currentPath, currentDirectory, FILE, &msg);
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
                close(fd[READ]);
            } else {
                close(fd[READ]);
                dup2(fd[WRITE], 1);
                close(fd[WRITE]);
                execlp("find", "find",  dir, "-mindepth", "1", "-type", "f", NULL);
            }
            free(currentPath);
        }
    } else {
        performInsert(dir, currentDirectory, FILE, &msg);
    }
    return msg;
}

int callBashProcess(char *toRtn, char *argv[]){
    int bytesRead = 1;
    char charRead = 0;
    int counter = 0;
    int fd[2];
    int rc_t = SUCCESS;
    pipe(fd);
    int f = fork();
    if (f > 0) {
        close(fd[WRITE]);
        while(bytesRead > 0){
            bytesRead = readChar(fd[READ], &charRead);
            if(bytesRead > 0){
                if(charRead != '\n'){
                    toRtn[counter] = charRead;
                    counter++;
                }
            }
        }
        if(bytesRead == -1){
            rc_t = READ_FAILURE;
        }
        toRtn[counter] = '\0';
        close(fd[READ]);
    } else if (f == 0) {
        close(fd[READ]);
        dup2(fd[WRITE], 1);
        close(fd[WRITE]);
        int rc_ex = execvp(argv[0], argv);
        if(rc_ex < 0){
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
        //TODO... SUICIDE
    } else {
        //TODO... Do something about not able to fork
        printError("The program couldn't execute a fork");
    }
    return rc_t;
}

int checkFileType(char *name){
    int rc_t = SUCCESS;
    char *command = (char *) malloc(sizeof(char)*(PATH_MAX*2 + 150));
    rc_t = checkAllocationError(command);
    if (rc_t < 0) {
        //TODO... HARAKIRI PROBABLY
        printError("I can't allocate memory");
    } else {
        sprintf(command, "([ -f '%s' ] && echo '0') || ([ -d '%s' ] && echo '1') || echo '2'", name, name);
        char *argv[4] = {"sh", "-c", command,  NULL};
        char type[2];
        rc_t = callBashProcess(type, argv);
        if(rc_t == SUCCESS){
            sscanf(type, "%d", &rc_t);
        }
        free(command);
    }
    return rc_t;
}

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

TreeNode precomputeStartingDirectory(Tree fs, TreeNode currentDirectory, char *path, int *toSkip){
    TreeNode toRtn = NULL;
    *toSkip = 0;
    int keepGoing = 0;
    int counter = 3;
    //printf("path: %s\n", path);
    if(path != NULL && fs != NULL && currentDirectory != NULL){ //Verify if everything is ok
        if(path[0] == '/'){ //if my path start with / then i'm talking about an Absolute Path
            toRtn = getRoot(fs);
            *toSkip = 1;
        } else if(path[0] == '.'){ //if my path starts with a dot...
            if(path[1] == '.'){ //and is followed by a dot...
                if(path[2] == '/'){ //then I need to go up but...
                    toRtn = currentDirectory->parent;
                    while(path[counter] == '.' && keepGoing == 0){ //I still need to check if I need to go up more
                        if(path[counter+1] == '.'){
                            if(path[counter+2] == '/'){
                                counter += 3;
                                toRtn = toRtn->parent;
                                if(toRtn == NULL){
                                    keepGoing = -1;
                                    printError("Something went terribly wrong in the path analysis so we'll attach it to the root");
                                    toRtn = getRoot(fs);
                                }
                            }
                        } else { //I'm arrived at the rigth level
                            keepGoing = -1;
                        }
                    }    
                    *toSkip = counter;
                } else {// I didn't even know it could be possible to create a folder called .......something
                    printInfo("You created a monster of a folder but I still handled the Error");
                    toRtn = currentDirectory;
                }
            } else {
                if(path[1] == '/'){ //If the first dot is followed by a slash then I'm talking about the folder I'm in
                    *toSkip = 2;
                } //Otherwise I'm talking about an hidden folder
                toRtn = currentDirectory;
            }
        } else {//In any other case I'm talking about a folder or a file inside my current folder
            toRtn = currentDirectory;
        }
    }
    //printf("devo skippare %d carattere/i e la working directory e' %s\n", *toSkip, ((FileInfo)toRtn->data)->name);
    return toRtn;
}

int readFileToAnalyze(char *cwd, Tree fs, TreeNode currentDirectory){
    char *path = (char *) malloc(sizeof(char)*PATH_MAX);
    int rc_t = SUCCESS;
    int rc_rd = SUCCESS;
    int rc_cf = SUCCESS;
    int rc_of = SUCCESS;
    int rc_cp = SUCCESS;
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
            rc_cf = checkFileType(path);
            int stringLength = strlen(path);
            if (rc_cf == DIRECTORY){
                if (path[stringLength - 1] != '/'){
                    path[stringLength] = '/';
                    path[stringLength + 1] = '\0';
                }
            }
            if (rc_cf == DIRECTORY || rc_cf == FILE){
                TreeNode workingDirectory = precomputeStartingDirectory(fs, currentDirectory, path, &toSkip);
                printf("path prima del save file: %s\n", path);
                rc_t = saveFiles(fs, workingDirectory, path, toSkip, rc_cf);
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
    free(path);
    return rc_t;
}

Tree fs = NULL;

void sighandle_int(int sig){
    destroyTree(fs);
    signal(SIGINT, SIG_DFL);
}

int main(){
    signal(SIGCHLD, SIG_IGN);
    signal(SIGINT, sighandle_int);
    char cwd[PATH_MAX];
    int rc_t = SUCCESS;
    TreeNode currentDirectory = NULL;
    getcwd(cwd, sizeof(cwd));
    if(cwd != NULL && rc_t == SUCCESS){ // If i can't access the program current folder then abort
        //CANNOT MOVE THIS CODE (AT LEAST IF YOU DON'T WANT TO DEAL WITH COPY OF POINTER)
        char *rootName = (char *)malloc(sizeof(char)*2);
        int rc_ca = checkAllocationError(rootName);
        if (rc_ca < 0) {
            //TODO... HARAKIRI PROBABLY
            printError("I can't allocate memory");
        } else {
            strcpy(rootName, "/");
            int rc_tc = SUCCESS;
            int rc_nc = SUCCESS;
            FileInfo rootData = newFileInfo((void *)rootName, DIRECTORY, &rc_nc);
            fs = newTree((void *)rootData, &rc_tc, destroyFileInfo, NULL);
            if(rc_tc != SUCCESS || rc_nc != SUCCESS){
                char *msgErr = (char *)malloc(300);
                rc_ca = checkAllocationError(msgErr);
                if (rc_ca < 0) {
                    //TODO... HARAKIRI PROBABLY
                    printError("I can't allocate memory");
                } else {
                    sprintf(msgErr, "Can't create tree inside Analyzer: %d", getpid());
                    printInfo(msgErr);
                    free(msgErr);
                }
            } else {
                currentDirectory = performInsert(cwd, getRoot(fs), DIRECTORY, &rc_t);
                if(currentDirectory == NULL){
                    rc_t = FAILURE;
                }
            }
        }
        while(rc_t == SUCCESS){
            rc_t = readFileToAnalyze(cwd, fs, currentDirectory);
        }
        destroyTree(fs);
    } else {
        //HARAKIRI INCOMING
        rc_t = FAILURE;
    }
    return rc_t;
}