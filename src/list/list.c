#include "list.h"
#include <stdio.h>
#include "../config/config.h"

List newList() {
  List ret = NULL;
  ret = (List)malloc(sizeof(struct List));
  int rc_lm = checkAllocationError(ret);
  if (rc_lm == SUCCESS) {
    ret->head = NULL;
    ret->tail = NULL;
    ret->size = 0;
  }
  return ret;
}

int isEmptyList(const List list) {
  int ret = NOT_EMPTY;
  if (list->size <= 0) {
    ret = EMPTY;
  }
  return ret;
}

int enqueue(List list, void *data) {
  int ret = SUCCESS;
  Node nodo = (Node)malloc(sizeof(struct Node));
  ret = checkAllocationError(nodo);
  if (ret == SUCCESS) {
    nodo->data = data;
    nodo->next = NULL;
    nodo->prev = list->tail;
    if (isEmptyList(list) == EMPTY) {
      list->head = nodo;
    } else {
      list->tail->next = nodo;
    }
    list->tail = nodo;
    list->size++;
  }
  return ret;
}

int push(List list, void *data) {
  int ret = SUCCESS;
  Node nodo = (Node)malloc(sizeof(struct Node));
  ret = checkAllocationError(nodo);
  if (ret == SUCCESS) {
    nodo->data = data;
    nodo->prev = NULL;
    nodo->next = list->head;
    if (isEmptyList(list) == EMPTY) {
      list->tail = nodo;
    } else {
      list->head->prev = nodo;
    }
    list->head = nodo;
    list->size++;
  }
  return ret;
}

void printList(const List list, void toString(void *)) {
  Node tmp_node = list->head;
  if (isEmptyList(list) == EMPTY) {
    printf("The list is empty\n");
  } else {
    printf("In the list there are %d element: \n", list->size);
    while (tmp_node != NULL) {
      printf("El: ");
      toString(tmp_node->data);
      tmp_node = tmp_node->next;
    }
    printf("End of the list\n");
  }
}

void destroyAllNode(List list, void deleteData(void *)) {
  Node tmp_node;
  Node tmp_precedente;
  if (isEmptyList(list) == NOT_EMPTY) {
    tmp_node = list->head;
    tmp_precedente = NULL;
    while (tmp_node != NULL) {
      tmp_precedente = tmp_node;
      tmp_node = tmp_node->next;
      deleteData(tmp_precedente->data);
      free(tmp_precedente);
      list->size--;
    }
    list->head = NULL;
    list->tail = NULL;
  }
}

void destroyList(List list, void deleteData(void *)) {
  if (list != NULL) {
    destroyAllNode(list, deleteData);
    free(list);
  }
}

int pop(List list) {
  int ret = SUCCESS;
  if(list != NULL){
    Node nodo;
    if (isEmptyList(list) == EMPTY) {
      ret = FAILURE;
    } else {
      nodo = list->head;
      list->head = nodo->next;
      list->size--;
      if (isEmptyList(list) == EMPTY) {
        list->tail = NULL;
      } else {
        list->head->prev = NULL;
      }
      free(nodo);
    }
  }else{
    ret = FAILURE;
  }
  return ret;
}

void *front(const List list) {
  void *data = NULL;
  if (isEmptyList(list) == NOT_EMPTY) {
    data = list->head->data;
  }
  return data;
}

int dequeue(List list) {
  int ret = SUCCESS;
  Node nodo;
  if (isEmptyList(list) == EMPTY) {
    ret = FAILURE;
  } else {
    nodo = list->tail;
    list->tail = nodo->prev;
    list->size--;
    if (isEmptyList(list) == EMPTY) {
      list->head = NULL;
    } else {
      list->tail->next = NULL;
    }
    free(nodo);
  }
  return ret;
}

void *tail(const List list) {
  void *data = NULL;
  if (isEmptyList(list) == NOT_EMPTY) {
    data = list->tail->data;
  }
  return data;
}

int isIn(const List list, void *data, int isEqual(void *, void *)) {
  int found = FAILURE;
  Node tmp_node = list->head;
  while (found == FAILURE && tmp_node != NULL) {
    if (isEqual(tmp_node->data, data) == SUCCESS) {
      found = SUCCESS;
    }
    if (found == FAILURE) {
      tmp_node = tmp_node->next;
    }
  }
  return found;
}

void *getData(const List list, void *data, int isEqual(void *, void *)) {
  void *ret = NULL;
  int found = FAILURE;
  Node tmp_node = list->head;
  while (found == FAILURE && tmp_node != NULL) {
    if (isEqual(tmp_node->data, data) == SUCCESS) {
      found = SUCCESS;
      ret = tmp_node->data;
    }
    if (found == FAILURE) {
      tmp_node = tmp_node->next;
    }
  }
  return ret;
}

int deleteAtIndex(List list, const int index, void deleteData(void *)) {
  int deleted = FAILURE;
  int currentSize = list->size;
  if (!(currentSize - 1 < index || index < 0)) {
    Node tmp_node = list->head;
    Node prev_node = NULL;
    Node next_node = NULL;
    int counter = index;
    while (counter != 0) {
      tmp_node = tmp_node->next;
      counter--;
    }
    prev_node = tmp_node->prev;
    next_node = tmp_node->next;
    if (prev_node != NULL) {
      prev_node->next = next_node;
    } else {
      list->head = next_node;
    }
    if (next_node != NULL) {
      next_node->prev = prev_node;
    } else {
      list->tail = prev_node;
    }
    list->size--;
    deleteData(tmp_node->data);
    free(tmp_node);
    deleted = SUCCESS;
  }
  return deleted;
}

int removeNode(const List list, void *data, int isEqual(void *, void *)) {
  int deleted = FAILURE;
  int found = FAILURE;
  Node tmp_node = list->head;
  Node prev_node = NULL;
  Node next_node = NULL;
  while (found == FAILURE && tmp_node != NULL) {
    if (isEqual(tmp_node->data, data) == SUCCESS) {
      found = SUCCESS;
      prev_node = tmp_node->prev;
      next_node = tmp_node->next;
      if (prev_node != NULL) {
        prev_node->next = next_node;
      } else {
        list->head = next_node;
      }
      if (next_node != NULL) {
        next_node->prev = prev_node;
      } else {
        list->tail = prev_node;
      }
      list->size--;
      free(tmp_node);
      deleted = SUCCESS;
    }
    if (found == FAILURE) {
      tmp_node = tmp_node->next;
    }
  }
  return deleted;
}

int detachNodeFromList(List list, Node node){
  int rc_t = SUCCESS;
  Node prev_node = NULL;
  Node next_node = NULL;
  if(node != NULL){
    prev_node = node->prev;
    next_node = node->next;
    if (prev_node != NULL) {
      prev_node->next = next_node;
    } else {
      list->head = next_node;
    }
    if (next_node != NULL) {
      next_node->prev = prev_node;
    } else {
      list->tail = prev_node;
    }
    list->size--;
  } else {
    rc_t = FAILURE;
  }
  free(node);
  return rc_t;
}

int deleteNode(const List list, void *data, int isEqual(void *, void *),
               void deleteData(void *)) {
  int deleted = FAILURE;
  int found = FAILURE;
  Node tmp_node = list->head;
  Node prev_node = NULL;
  Node next_node = NULL;
  while (found == FAILURE && tmp_node != NULL) {
    if (isEqual(tmp_node->data, data) == SUCCESS) {
      found = SUCCESS;
      prev_node = tmp_node->prev;
      next_node = tmp_node->next;
      if (prev_node != NULL) {
        prev_node->next = next_node;
      } else {
        list->head = next_node;
      }
      if (next_node != NULL) {
        next_node->prev = prev_node;
      } else {
        list->tail = prev_node;
      }
      list->size--;
      deleteData(tmp_node->data);
      free(tmp_node);
      deleted = SUCCESS;
    }
    if (found == FAILURE) {
      tmp_node = tmp_node->next;
    }
  }
  return deleted;
}

int swap(List first, List second) {
  int ret = FAILURE;
  int size = 0;
  Node node = NULL;
  if (first != NULL && second != NULL) {
    size = first->size;
    first->size = second->size;
    second->size = size;

    node = first->head;
    first->head = second->head;
    second->head = node;

    node = first->tail;
    first->tail = second->tail;
    second->tail = node;

    ret = SUCCESS;
  }
  return ret;
}

void map(List list, void function(void *)) {
  Node tmp_node = list->head;
  if (isEmptyList(list) == NOT_EMPTY) {
    while (tmp_node != NULL) {
      function(tmp_node->data);
      tmp_node = tmp_node->next;
    }
  }
}

int concat(List dst, List src) {
  int rc_t = SUCCESS;
  if (dst != NULL && src != NULL) {
    if(isEmptyList(dst) == EMPTY){
      swap(dst, src);
    }else{
      if(isEmptyList(src) == NOT_EMPTY){
        if(dst->tail != NULL){
          dst->tail->next = src->head;
        }
        if(src->head != NULL){
          src->head->prev = dst->tail;
        }
        dst->tail = src->tail;
        dst->size += src->size;
        // Empty the src List
        src->head = NULL;
        src->tail = NULL;
        src->size = 0;
      }
    }
  } else {
    rc_t = FAILURE;
  }
  return rc_t;
}
