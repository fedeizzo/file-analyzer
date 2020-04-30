#ifndef __QUEUE_H_
#define __QUEUE_H_

#include<stdio.h>
#include<stdlib.h>

typedef char bool;
#define true 1
#define false 0

#define MALLOC_FAILURE -1

typedef struct Node {
	void *data;
	struct Node *next;
	struct Node *prev;
} Node;

typedef struct List{
	int size;
	Node *head;
	Node *tail;
} List;

/**
 * Function that initialize the list structure
 */
void initList(List *list);
/**
 * Function that check if the list is empty
 */
bool isEmptyList(const List *list);
/*
 * Funzione che aggiunge in coda
 */
int enqueue(List *list, void *data);
/**
 * Funzione che aggiunge in testa
 */
int push(List *list, void *data);
/**
 * Printing a list
 */
void printList(const List *list);
/*
 * Delete all the elements in a list
 */
void destroyList(List *list);
/**
 * Dequeue an element
 */
bool dequeue(List *list);
/**
 * Element in front
 */
void * front(const List *list);
/*
 * Pop an element
 */
bool pop(List *list);
/**
 * Tail element
 */
void * tail(const List *list);
/**
 * Search for an element
 */
bool isIn(List *list, const void *data);
#endif
