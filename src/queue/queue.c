#include "queue.h"

void initList(List *list) {
  list->head = NULL;
  list->tail = NULL;
  list->size = 0;
}

bool isEmptyList(const List *list) {
  bool ret = false;
  if (list->size <= 0) {
    ret = true;
  }
  return ret;
}

int enqueue(List *list, void *data) {
  int ret = 0;
  Node *nodo = (Node *)malloc(sizeof(Node));
  if (nodo == NULL) {
    perror("Failure allocating memory\n");
    ret = MALLOC_FAILURE;
  } else {
    nodo->data = data;
    nodo->next = NULL;
    nodo->prev = list->tail;
    if (isEmptyList(list)) {
      list->head = nodo;
    } else {
      list->tail->next = nodo;
    }
    list->tail = nodo;
    list->size++;
  }
  return ret;
}

int push(List *list, void *data) {
  int ret = 0;
  Node *nodo = (Node *)malloc(sizeof(Node));
  if (nodo == NULL) {
    perror("Malloc failure\n");
    ret = MALLOC_FAILURE;
  } else {
    nodo->data = data;
    nodo->prev = NULL;
    nodo->next = list->head;
    if (isEmptyList(list)) {
      list->tail = nodo;
    } else {
      list->head->prev = nodo;
    }
    list->head = nodo;
    list->size++;
  }
  return ret;
}

void printList(const List *list) {
  Node *tmp_node = list->head;
  if (isEmptyList(list)) {
    printf("The list is empty\n");
  } else {
    printf("In the list there are %d element: \n", list->size);
    while (tmp_node != NULL) {
      printf("El: %d\n", *((int *)tmp_node->data));
      tmp_node = tmp_node->next;
    }
    printf("End of the list\n");
  }
}

void destroyList(List *list) {
  Node *tmp_node;
  Node *tmp_precedente;
  if (!isEmptyList(list)) {
    tmp_node = list->head;
    tmp_precedente = NULL;
    while (tmp_node != NULL) {
      tmp_precedente = tmp_node;
      tmp_node = tmp_node->next;
      free(tmp_precedente);
      list->size--;
    }
    list->head = NULL;
    list->tail = NULL;
  }
}

bool dequeue(List *list) {
  bool ret = true;
  Node *nodo;
  if (isEmptyList(list)) {
    ret = false;
  } else {
    nodo = list->head;
    list->head = nodo->next;
    list->size--;
    if (list->size == 0) {
      list->tail = NULL;
    } else {
      list->head->prev = NULL;
    }
    free(nodo);
  }
  return ret;
}

void *front(const List *list) {
  void *data = NULL;
  if (!isEmptyList(list)) {
    data = list->head->data;
  }
  return data;
}

bool pop(List *list) {
  bool ret = true;
  Node *nodo;
  if (isEmptyList(list)) {
    ret = false;
  } else {
    nodo = list->tail;
    list->tail = nodo->prev;
    list->size--;
    if (isEmptyList(list)) {
      list->head = NULL;
    } else {
      list->tail->next = NULL;
    }
    free(nodo);
  }
  return ret;
}

void *tail(const List *list) {
  void *data = NULL;
  if (!isEmptyList(list)) {
    data = list->tail->data;
  }
  return data;
}

bool isIn(List *list, const void *data) {
  bool found = false;
  Node *tmp_node = list->head;
  while (!found && tmp_node != NULL) {
    if (tmp_node->data == data) {
      found = true;
    }
    tmp_node = tmp_node->next;
  }
  return found;
}

int random_epic42069(int min, int max) { return (rand() % max - min) + min; }
