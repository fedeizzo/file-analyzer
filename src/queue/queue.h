#ifndef __QUEUE_H_
#define __QUEUE_H_

#include<stdio.h>
#include<stdlib.h>

#define MALLOC_FAILURE -2
#define NOT_EMPTY 0
#define EMPTY -1
#define SUCCESS 0
#define FAILURE -1

typedef struct Node {
	void *data;
	struct Node *next;
	struct Node *prev;
} Node;

typedef struct List{
	int size;
	Node *head;
	Node *tail;
} *List;

/**
 * Function that create and initialize the function, if fails, returns NULL
 */
List newList();
/**
 * Function that check if the list is empty
 */
int isEmptyList(const List list);
/*
 * Funzione che aggiunge in coda
 */
int enqueue(List list, void *data);
/**
 * Funzione che aggiunge in testa
 */
int push(List list, void *data);
/**
 * Printing a list
 */
void printList(const List list, void toString(void *));
/*
 * Delete all the elements in a list
 */
void destroyAllNode(List list, void deleteData(void *));
/**
 * Dequeue an element
 */
int dequeue(List list);
/**
 * Element in front
 */
void * front(const List list);
/*
 * Pop an element
 */
int pop(List list);
/**
 * Tail element
 */
void * tail(const List list);
/**
 * Search an Element
 */
int isIn(const List list, void *data, int isEqual(void *, void *));
/**
 * Get data from the list which match based on the function passed as an argument 
 */
void * getData(const List list, void *data, int isEqual(void *, void *));
/**
 * delete a node at specific index
 */
int deleteAtIndex(List list, const int index, void deleteData(void *));
/**
 * function used to delete a specific element from the list which match the function passed as argument
 */
int deleteNode(const List list, void *data, int isEqual(void *, void *), void deleteData(void *));
/**
 * function that swaps two lists and return SUCCESSES or FAILURE if at least one of them is NULL
 */
int swap(List first, List second);
/**
 * function that removes an element if it matches one passed as an argument based on the argument function
 * and returns SUCCESS if the element is found or FAILURE if isn't in the list
 */
int removeNode(List list, void *data, int isEqual(void *, void *));
/**
 * function that destroy all the node and all the data in the list and after that free the list itself
 * needs a function to delete the other malloc inside the void *data
 */
void destroyList(List list, void deleteData(void *));
#endif
