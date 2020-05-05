#ifndef __TREE_H_
#define __TREE_H_
#include "../list/list.h"

#define NULL_POINTER -3
#define OVERRIDING_TREE -4

#define MAXLENGHT 300

/**
 * Handles a node of the tree
 *
 * fields:
 *    void *data: the data of the node
 *    List children: the list which contains the children node of the current node
 *    struct TreeNode *parent: parent node
 *    void (*destroy)(void *) : specific destructor of the node
 */
typedef struct TreeNode {
    void* data;
    List children;
    struct TreeNode *parent;
    void (*destroy)(void *);
} *TreeNode;

/**
 * Handles a tree
 *
 * fields:
 *    TreeNode root: root node of the tree
 */
typedef struct Tree {
    TreeNode root;
} *Tree;

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
Tree newTree(void *data, int *msg, void destroy(void *));

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
TreeNode newTreeNode(TreeNode parent, void *data, int *msg, void destroy(void *));

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
int initTreeNode(TreeNode child, TreeNode parent, void *data, void destroy(void *));

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
int linkChild(TreeNode parent, TreeNode child);

/*
 * Destroy the tree
 *
 * args:
 *   Tree tree: tree which will be destroyed
 */
void destroyTree(Tree tree);

/*
 * Destroy a TreeNode by deleting all it's children and their infos
 *
 * args:
 *   void *node: node whose children will be deleted
 */
void destroyChildren(void *node);

/*
 * Destroy node's data
 *
 * args:
 *   void *node: node whose data will be deleted
 */
void destroyNode(void *node);

#endif