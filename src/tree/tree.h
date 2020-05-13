#ifndef __TREE_H_
#define __TREE_H_
#include "../list/list.h"
#include "../wrapping/wrapping.h"

#define NULL_POINTER -4
#define OVERRIDING_TREE -5

/**
 * Handles a node of the tree
 *
 * fields:
 *    void *data: the data of the node
 *    List children: the list which contains the children node of the current node
 *    struct TreeNode *parent: parent node
 */
typedef struct TreeNode {
    void* data;
    List children;
    struct TreeNode *parent;
} *TreeNode;

//TODO... DELETE toCompare FROM THE TREE
/**
 * Handles a tree
 *
 * fields:
 *    TreeNode root: root node of the tree
 *    void (*destroy)(void *) : specific destructor of the tree
 */
typedef struct Tree {
    TreeNode root;
    void (*destroy)(void *);
    int (*toCompare)(void *, void *);
} *Tree;

//TODO... DELETE toCompare FROM THE TREE
/**
 * Creates and initializes the Tree 
 *
 * args:
 *    void *data: the data to inset
 *    int *msg: exit code
 *    void destroy(void *): specific destructor of the node
 * 
 * returns:
 *    the tree created in case of success, otherwise NULL
 */
Tree newTree(void *data, int *msg, void destroy(void *), int toCompare(void *, void *));

/*
 * Return the root node of a tree
 *
 * args:
 *   Tree tree: Tree
 *
 * returns:
 *    Root of the tree in case of success, otherwise NULL
 */
TreeNode getRoot(Tree tree);

/*
 * Creates and initializes a TreeNode without attaching it to the parent node
 *
 * args:
 *   TreeNode parent: parent TreeNode
 *   void *data: data of the node
 *   int *msg: return code
 *   void destroy(void *): specific desctructor of the node
 *
 * returns:
 *    New TreeNode in case of success, otherwise NULL
 */
TreeNode newTreeNode(TreeNode parent, void *data, int *msg);

/*
 * Init a TreeNode setting the parent node, the data and the destroy field
 *
 * args:
 *   TreeNode child: child node that will be initialized
 *   TreeNode parent: parent node reference
 *   void *data: data of the node
 *   void destroy(void *): specific destructor of the node
 *
 * returns:
 *    0 in case of success, MALLOC_FAILURE or NULL_POINTER otherwise
 */
int initTreeNode(TreeNode child, const TreeNode parent, void *data);

/*
 * Link a child node to it's parent
 *
 * args:
 *   TreeNode child: child node which will be linked
 *   TreeNode parent: parent node reference
 *
 * returns:
 *    0 in case of success, NULL_POINTER otherwise
 */
int linkChild(TreeNode parent, const TreeNode child);

/*
 * Destroy the tree
 *
 * args:
 *   Tree tree: tree which will be destroyed
 * 
 * returns:
 *    0 in case of success, NULL_POINTER, MALLOC_FAILURE or UNEXPECTED_LIST_ERROR otherwise   
 */
int destroyTree(Tree tree);

/*
 * Destroy node's data
 *
 * args:
 *   void *node: node whose data will be deleted
 */
void destroyNode(void *toDestroy, void destroy(void *));

//TODO... REMOVE FROM TREE
/*
 * compare two TreeNodes' datas with a specific function witch
 *
 * args:
 *   TreeNode first: first TreeNode to be checked
 *   TreeNode second: second TreeNode which will be checked with the first one
 *   toCompare: function which takes two void *data and will compare them togheter. It must return
 *      0 if datas are equals or -1 otherwise
 * 
 * returns:
 *    0 if they are equal, -1 if they're not or NULL_POINTER if at least one of them is NULL  
 */
int compareNode(TreeNode first, TreeNode second, int toCompare(void *, void *));
#endif