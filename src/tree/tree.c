#include <stdio.h>
#include <stdlib.h>
#include "tree.h"

/**
 * return NULL if malloc failed, needs destructor, data and a pointer to an integer for end message of allocations
 */
Tree newTree(void *data, int *msg, void destroy(void *)){
    Tree tree = (Tree) malloc (sizeof(struct Tree));
    *msg = SUCCESS;
    if(tree != NULL){
        tree->root = newTreeNode(NULL, data, msg, destroy);
    } else {
        *msg = MALLOC_FAILURE;
    }    
    return tree;
}

TreeNode getRoot(Tree tree){
    if(tree != NULL){
        return tree->root;
    }
}

TreeNode newTreeNode(TreeNode parent, void *data, int *msg, void destroy(void *)){
    TreeNode child = (TreeNode) malloc (sizeof(struct TreeNode));
    *msg = SUCCESS;
    if(child != NULL){
        *msg = initTreeNode(child, parent, data, destroy);
    } else {
        *msg = MALLOC_FAILURE;
    }
    return child;    
}

int initTreeNode(TreeNode child, TreeNode parent, void *data, void destroy(void *)){
    int rtn = SUCCESS;
    if(child != NULL){
        child->children = newList();
        if(child->children == NULL){
            rtn = MALLOC_FAILURE;
        } else {
            child->parent = parent;
            child->data = data;
            child->destroy = destroy;
        }    
    } else {
        rtn = NULL_POINTER;
    }
    return rtn;
}

int linkChild(TreeNode parent, TreeNode child){
    int rtn = SUCCESS;
    if(parent != NULL && child != NULL){
        rtn = push(parent->children, child);
    } else {
        rtn = NULL_POINTER;
    }
    return rtn;
}

void destroyTree(Tree tree){
    if(tree != NULL){
        TreeNode root = getRoot(tree);
        if(root!=NULL){
            destroyChildren((void *)root);
            root->destroy(root->data);
            free(root);
        }
        free(tree);
    }
}

void destroyChildren(void *node){
    TreeNode n = (TreeNode) node;
    if(n != NULL){
        map(n->children, destroyChildren);
        destroyList(n->children, destroyNode);
    }
}

void destroyNode(void *node){
    TreeNode n = (TreeNode) node;
    n->destroy(n->data);
    free(node);
}

/*
void printTree(Tree tree){
    if(tree != NULL){
        printf("Tree:\n");
        printTreeNode(tree->root);
    }
}

void printTreeNode(void *node){
    TreeNode n = (TreeNode) node;
    if(node != NULL){
        map(n->children, printTreeNode);
	    printf("Node: %d\n", *((int *)n->data));
    }
}

void destroyInt(void *data){
    printf("currently destroying %d\n", *((int *)data));
	free(data);
}

int main(){
    int i = 0;
    int c;
    int *toSearch;
    int *el;
	int *sono_la_radice_epica = (int *) malloc(sizeof(int));
    *sono_la_radice_epica = 0;
	int msg = SUCCESS;
	Tree tree = newTree(sono_la_radice_epica, &msg, destroyInt);
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
        nodo = newTreeNode(toAdd, (void *)el, &msg, destroyInt);
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
