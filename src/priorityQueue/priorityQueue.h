#ifndef __PRIORITYQUEUE_H__
#define __PRIORITYQUEUE_H__

#include "../config/config.h"

/**
 * Holds data and priority for an element of the priority queue
 *
 * fields:
 *    int priority: the priority
 *    void *data: the data
 */
typedef struct {
  int priority;
  void *data;
} Element;

/**
 * Holds the priority queue
 *
 * fields:
 *    Element *nodes: array of Element
 *    int len: amount of element inside the queue
 *    int size: the max size of the queue
 */
typedef struct PriorityQueueStr{
  Element *nodes;
  int len;
  int size;
} * PriorityQueue;


PriorityQueue newPriorityQueue();

/**
 * Inserts data inside a the heap structure according to the priority
 *
 * args:
 *    PriorityQueue q: the priority queue where data is insered
 *    int priority: self explainatory
 *    void *data: data for insert operation
 *
 * retuns:
 *    0 in case of success, otherwise -1
 */
int pushPriorityQueue(PriorityQueue q, int priority, void *data);

/**
 * Removes an element from the priority queue passed
 *
 * args:
 *    PriorityQueue q: the priority queue for remove operation
 *
 * returns:
 *    the data removed in case of success, otherwise NULL
 */
void *popPriorityQueue(PriorityQueue q);

/**
 * Destroys an element
 *
 * args:
 *    Element el: the element for destroy operation
 *    void deleteData(void *): the funcion used for destroy operation
 */
void destroyElement(Element el, void deleteData(void *));

/**
 * Destroys a priority queue
 *
 * args:
 *    PriorityQueue q: the priority queue for destroy operation
 *    void deleteData(void *): the funcion used for destroy operation
 */
void destroyPriorityQueue(PriorityQueue q, void deleteData(void *));

/**
 * Swaps two priority queues and returns
 *
 * args:
 *    PriorityQueue first: first priority queue
 *    PriorityQueue second: second priority queue
 *
 * returns:
 *    0 in case of success, otherwise -1
 */
int swapPriorityQueue(PriorityQueue first, PriorityQueue second);
#endif
