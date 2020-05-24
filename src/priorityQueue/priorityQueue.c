#include <stdio.h>
#include <stdlib.h>

#include "./priorityQueue.h"
#include "../wrapping/wrapping.h"
#include "../config/config.h"

PriorityQueue newPriorityQueue(){
  PriorityQueue q = (PriorityQueue) calloc(1, sizeof(struct PriorityQueueStr));
  checkAllocationError(q);
  return q;
}

int pushPriorityQueue(PriorityQueue q, int priority, void *data) {
  int rc_t = 0;
  if (q->len + 1 >= q->size) {
    q->size = q->size ? q->size * 2 : 4;
    Element *newPointer =
        (Element *)realloc(q->nodes, q->size * sizeof(Element));
    if (newPointer != NULL)
      q->nodes = newPointer;
    else
      rc_t = MALLOC_FAILURE;
  }

  int i = q->len + 1;
  int j = i / 2;
  while (i > 1 && q->nodes[j].priority > priority) {
    q->nodes[i] = q->nodes[j];
    i = j;
    j = j / 2;
  }

  q->nodes[i].priority = priority;
  q->nodes[i].data = data;
  q->len++;

  return rc_t;
}

void *popPriorityQueue(PriorityQueue q) {
  int i, j, k;
  void *returnValue;
  if (q->len == 0) {
    returnValue = NULL;
  } else {
    returnValue = q->nodes[1].data;

    q->nodes[1] = q->nodes[q->len];

    q->len--;

    i = 1;
    while (i != q->len + 1) {
      k = q->len + 1;
      j = 2 * i;
      if (j <= q->len && q->nodes[j].priority < q->nodes[k].priority) {
        k = j;
      }
      if (j + 1 <= q->len && q->nodes[j + 1].priority < q->nodes[k].priority) {
        k = j + 1;
      }
      q->nodes[i] = q->nodes[k];
      i = k;
    }
  }

  return returnValue;
}

void destroyElement(Element el, void deleteData(void *)) {
  deleteData(el.data);
}

void destroyPriorityQueue(PriorityQueue q, void deleteData(void *)) {
  int i;
  int len = q->len;

  for (i = 1; i < len + 1; i++) {
    Element el = q->nodes[i];
    destroyElement(el, deleteData);
  }
  free(q);
}

int swapPriorityQueue(PriorityQueue first, PriorityQueue second){
  //TODO cambiare questo errore di ritorno
  int ret = -1;
  Element *nodes;
  int size = 0;
  int len = 0;
  Element *elements = NULL;
  if (!(first == NULL || second == NULL)) {
    size = first->size;
    len = first->len;

    first->size = second->size;
    first->len = second->len;
    second->size = size;
    second->len = len;

    elements = first->nodes;
    first->nodes = second->nodes;
    second->nodes = elements;

    ret = SUCCESS;
  }
  return ret;
}