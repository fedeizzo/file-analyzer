#ifndef __QUEUE_H_
#define __QUEUE_H_

#include<stdio.h>
#include<stdlib.h>

#define MALLOC_FAILURE -2
#define NOT_EMPTY 0
#define EMPTY -1
#define SUCCESS 0
#define FAILURE -1

/**
 * Handles a node of the list
 *
 * fields:
 *    void *data: the data of the node
 *    struct Node *next: next node
 *    struct Node *prev: prev node
 */
typedef struct Node {
	void *data;
	struct Node *next;
	struct Node *prev;
} Node;

/**
 * Handles a list
 *
 * fields:
 *    int size: the size of the list
 *    Node *head: the head of the list
 *    Ndoe *tail: the tail of the list
 */
typedef struct {
	int size;
	Node *head;
	Node *tail;
} *List;

/**
 * Creates and initializes the List 
 *
 * returns:
 *    the list created in case of success, otherwise NULL
 */
List newList();

/**
 * Checks if the list is empty
 *
 * returns:
 *    0 in case is not empty, otherwise -1
 */
int isEmptyList(const List list);

/*
 * Adds element in tail position
 *
 * args:
 *    List list: the list in which insert the element
 *    void *data: the data to inset
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int enqueue(List list, void *data);

/*
 * Adds element in tail position
 *
 * args:
 *    List list: the list in which insert the element
 *    void *data: the data to inset
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int push(List list, void *data);

/*
 * Prints the list
 *
 * args:
 *    List list: the list in which insert the element
 *    void *toString: high order function to print the data
 */
void printList(const List list, void toString(void *));

/**
 * Deletes all the elements in a list
 *
 * args:
 *    List list: the list from which elements are removed
 *    void deleteData(void *): high order function to deallocate correctly the memory for data
 */
void destroyAllNode(List list, void deleteData(void *));

/**
 * Dequeue an element
 */
/**
 * Removes an element from head
 *
 * args:
 *    List list: the list from which element is removed
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int dequeue(List list);

/**
 * Returns the element in head position
 *
 * args:
 *    const List list:  the list from which element is got
 *
 * returns:
 *    the element in caso of success, otherwise NULL
 */
void * front(const List list);

/**
 * Remove the element in head position
 *
 * args:
 *    const List list:  the list from which element is got
 *
 * returns:
 *    the element in caso of success, otherwise NULL
 */
int pop(List list);

/**
 * Remove the element in tail position
 *
 * args:
 *    const List list:  the list from which element is got
 *
 * returns:
 *    the element in caso of success, otherwise NULL
 */
void * tail(const List list);

/**
 * Checks if the element is inside the list
 *
 * args:
 *    const List list: the list in which search
 *    void *data: the element for the research operation
 *    int isEqual(void*, void*): the function for the comparison
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int isIn(const List list, void *data, int isEqual(void *, void *));

/**
 * Returns the element from the list
 *
 * args:
 *    const List list: the list in which search
 *    void *data: the element for the research operation
 *    int isEqual(void*, void*): the function for the comparison
 *
 * returns:
 *    the element in case of success, otherwise NULL
 */
void * getData(const List list, void *data, int isEqual(void *, void *));
 
/**
 * Deletes a node at specific index
 *
 * args:
 *    List list: the list in which search
 *    const int index: the index in which the element is
 *    void deleteData(void *): high order function to deallocate correctly the memory for data
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int deleteAtIndex(List list, const int index, void deleteData(void *));
/**
 * function used to delete a specific element from the list which match the function passed as argument
 */
/**
 * Deletes an element from the list
 *
 * args:
 *    List list: the list in which remove operation is made
 *    void *data: the element for the remove operationg
 *    int isEqual(void*, void*): the function for the comparison
 *    void deleteData(void *): high order function to deallocate correctly the memory for data
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int deleteNode(const List list, void *data, int isEqual(void *, void *), void deleteData(void *));

/**
 * Swaps two list and returns
 *
 * args:
 *    List first: first list
 *    List second: second list
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int swap(List first, List second);

/**
 * Removes an elment from the list (if present)
 *
 * args:
 *    List list: the list from which remotion is made
 *    void *data: the data for the rimotion
 *    int isEqual(void *, void *): the function used for the comparison
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int removeNode(List list, void *data, int isEqual(void *, void *));

/**
 * Destroys all the ndoe and all the data associated with them. Needs a function
 * for associated data remotion
 *
 * args:
 *    List list: the list from which elments are removed
 *    void deleteData(void *): the function used to the rimotion of the data
 */
void destroyList(List list, void deleteData(void *));
#endif
