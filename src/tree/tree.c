#include <stdio.h>
#include <stdlib.h>
#include "tree.h"
#include "../config/config.h"

Tree newTree(void *data, int *msg, void destroy(void *), int toCompare(void *, void *)){
    Tree tree = (Tree) malloc (sizeof(struct Tree));
    *msg = checkAllocationError(tree); 
    if(*msg == SUCCESS){
        tree->root = newTreeNode(NULL, data, msg);
        tree->toCompare = toCompare;
        tree->destroy = destroy;
    }   
    return tree;
}

TreeNode getRoot(Tree tree){
    if(tree != NULL){
        return tree->root;
    }
}

TreeNode newTreeNode(TreeNode parent, void *data, int *msg){
    TreeNode child = (TreeNode) malloc (sizeof(struct TreeNode));
    *msg = checkAllocationError(child);
    if(*msg == SUCCESS){
        *msg = initTreeNode(child, parent, data);
    }
    return child;    
}

int initTreeNode(TreeNode child, const TreeNode parent, void *data){
    int rtn = SUCCESS;
    if(child != NULL){
        child->children = newList();    //newList print the allocation error in case of malloc failure
        if(child->children == NULL){
            rtn = MALLOC_FAILURE;
        } else {
            child->parent = parent;
            child->data = data;
        }    
    } else {
        rtn = NULL_POINTER;
    }
    return rtn;
}

int linkChild(TreeNode parent, const TreeNode child){
    int rtn = SUCCESS;
    if(parent != NULL && child != NULL){
        rtn = push(parent->children, child);
    } else {
        rtn = NULL_POINTER;
    }
    return rtn;
}

void destroyAllChildren(List list, void deleteData(void *), void destroyNode(void *, void deleteData(void *))) {
  Node tmp_node;
  Node tmp_precedente;
  if (isEmptyList(list) == NOT_EMPTY) {
    tmp_node = list->head;
    tmp_precedente = NULL;
    while (tmp_node != NULL) {
      tmp_precedente = tmp_node;
      tmp_node = tmp_node->next;
      destroyNode(tmp_precedente->data, deleteData);
      free(tmp_precedente);
      list->size--;
    }
    list->head = NULL;
    list->tail = NULL;
  }
}

void destroyParentList(List list, void deleteData(void *), void destroyNode(void *, void deleteData(void *))) {
  if (list != NULL) {
    destroyAllChildren(list, deleteData, destroyNode);
    free(list);
  }
}

int destroyTree(Tree tree){
    List toExamine = newList();
    List toDelete = newList();
    int rc_t = SUCCESS;
    if(toExamine != NULL && toDelete != NULL){
        if(tree != NULL){
            rc_t = enqueue(toExamine, (void *)getRoot(tree));
            if(rc_t < SUCCESS){
                rc_t = MALLOC_FAILURE;
            }
            TreeNode tmp = NULL;
            Node nodeList = NULL;
            while(rc_t == SUCCESS && isEmptyList(toExamine) == NOT_EMPTY){
                tmp = (TreeNode) tail(toExamine);
                rc_t = dequeue(toExamine);
                if(tmp != NULL){ //Solvable Error (if a node during execution is NULL then go to the next one)
                    if(rc_t == SUCCESS){
                        rc_t = push(toDelete, tmp);
                        if(rc_t == SUCCESS){
                            if(tmp->children != NULL){ //Solvable Error (destroyList does check if list is null)
                                if(isEmptyList(tmp->children) == NOT_EMPTY){ //Solvable Error (destroyList will not check node if is EMPTY)
                                    nodeList = tmp->children->head;
                                    while(nodeList != NULL && rc_t == SUCCESS){
                                        if(nodeList->data != NULL){ //Solvable Error: (destroyNode will handle if a TreeNode is NULL)
                                            rc_t = enqueue(toExamine, nodeList->data);
                                            nodeList = nodeList->next;
                                        }
                                    }
                                }
                            }
                        }
                    } else  {
                        rc_t = UNEXPECTED_LIST_ERROR;
                    }
                }
            }
            while(isEmptyList(toDelete) == NOT_EMPTY && rc_t == SUCCESS){
                tmp = (TreeNode) front(toDelete);
                rc_t = pop(toDelete);
                if(rc_t == SUCCESS && tmp != NULL){
                    destroyParentList(tmp->children, tree->destroy, destroyNode);
                } else {
                    rc_t = UNEXPECTED_LIST_ERROR;
                }
            }
            tmp = getRoot(tree);
            if(tmp != NULL){
                destroyNode((void *)tmp, tree->destroy);
            }
            free(tree);
        } else {
            rc_t = NULL_POINTER;
        }
        free(toExamine);
        free(toDelete);
    } else {
        rc_t = MALLOC_FAILURE;
    }
    return rc_t;
}

void destroyNode(void *toDestroy, void destroy(void *)){
    TreeNode n = (TreeNode) toDestroy;
    if(toDestroy != NULL){
        if(n->data != NULL){
            destroy(n->data);
        }
        free(n);
    }
}

int compareNode(TreeNode first, TreeNode second, int toCompare(void *, void *)){
    int rc_t = SUCCESS;
    if(first->data != NULL && second->data != NULL){
        rc_t = toCompare(first->data, second->data);
    } else {
        rc_t = NULL_POINTER;
    }
}
/*
void printTreeNode(void *node){
    TreeNode n = (TreeNode) node;
    if(node != NULL){
        map(n->children, printTreeNode);
	    printf("Node: %d\n", *((int *)n->data));
    }
}

void printTree(Tree tree){
    if(tree != NULL){
        printf("Tree:\n");
        printTreeNode((void *)tree->root);
    }
}

void destroyInt(void *data){
    printf("currently destroying %d\n", *((int *)data));
	free(data);
}

int toCompare_int(void *data1, void *data2){
    return *((int *) data1) == *((int *) data2);
}

int main(){
    int i = 0;
    int c;
    int *toSearch;
    int *el;
	int *sono_la_radice_epica = (int *) malloc(sizeof(int));
    *sono_la_radice_epica = 0;
	int msg = SUCCESS;
	Tree tree = newTree(sono_la_radice_epica, &msg, destroyInt, toCompare_int);
	TreeNode nodo = NULL;
    TreeNode last = NULL;
    TreeNode toAdd = getRoot(tree);
    while(1){
    printf("Inserisci il codice: \n");
    scanf("%d", &c);
    switch(c){
      case 1:
        last = nodo;
        el = (int*) malloc(sizeof(int));
        printf("Inserisci l'elemento da mettere nell'albero:\n");
        scanf("%d", el);
        nodo = newTreeNode(toAdd, (void *)el, &msg);
        break;
      case 2:
        toAdd = last;
        break;
      case 3:
        linkChild(toAdd, nodo);
        break;
      case 4:
        printTree(tree);
        break;
      case 5:
        destroyTree(tree);
        break;
      default:
        printf("I am in the default\n");
        destroyTree(tree);
        exit(0);
    }
  }
}*/
