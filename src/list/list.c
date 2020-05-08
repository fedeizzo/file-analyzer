#include "list.h"
#include <stdio.h>

List newList() {
  List ret = NULL;
  ret = (List)malloc(sizeof(struct List));
  if (ret != NULL) {
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
  int ret = 0;
  Node nodo = (Node)malloc(sizeof(struct Node));
  if (nodo == NULL) {
    perror("Failure allocating memory\n");
    ret = MALLOC_FAILURE;
  } else {
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
  int ret = 0;
  Node nodo = (Node)malloc(sizeof(struct Node));
  if (nodo == NULL) {
    perror("Malloc failure\n");
    ret = MALLOC_FAILURE;
  } else {
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
  Node nodo;
  if (isEmptyList(list) == EMPTY) {
    ret = FAILURE;
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

void *front(const List list) {
  void *data = NULL;
  if (!isEmptyList(list)) {
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
  if (!(first == NULL || second == NULL)) {
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

void map(List list, void function(void *)){
  Node tmp_node = list->head;
  if (isEmptyList(list) == NOT_EMPTY) {
    while (tmp_node != NULL) {
      function(tmp_node->data);
      tmp_node = tmp_node->next;
    }
  } 
}

/*
int int_eq(void *primo, void * secondo){
    return ((*(int *)primo) == (*(int *)secondo)) ? 0 : -1;
}

void toString_int(void * toPrint){
    printf("%d\n", *((int *)toPrint));
}

void delete_int(void *data){
    free(data);
}*/
/**
 * MAIN TO TEST
 */
/*
int main(){
  int i = 0;
  int c;
  int *toSearch;
  int *el;
  List l = newList();
  List l2 = newList();
  while(1){
    printf("Inserisci il codice: \n");
    scanf("%d", &c);
    switch(c){
      case 1:
        el = (int *)front(l);
        if(el != NULL){
          printf("%d\n", *(el));
        }
        dequeue(l);
        break;
      case 2:
        el = (int *)tail(l);
        if(el != NULL){
          printf("%d\n", *(el));
        }
        pop(l);
        break;
      case 3:
        el = (int*) malloc(sizeof(int));
        printf("Inserisci l'elemento da incodare:\n");
        scanf("%d", el);
        enqueue(l, (void*) el);
        break;
      case 4:
        el = (int*) malloc(sizeof(int));
        printf("Inserisci l'elemento da inserire in testa:\n");
        scanf("%d", el);
        push(l, (void*) el);
        break;
      case 5:
        toSearch = (int*) malloc(sizeof(int));
        printf("Searching for...\n");
        scanf("%d", toSearch);
        printf("%d\n ", isIn(l, (void *) toSearch, int_eq));
        break;
      case 6:
        printList(l, toString_int);
        break;
      case 7:
        toSearch = (int*) malloc(sizeof(int));
        printf("Which Element to return?...\n");
        scanf("%d", toSearch);
        void *obtained = getData(l, (void *) toSearch, int_eq);
        if(obtained!=NULL){
            printf("%d\n ", *((int *) obtained));
        }else{
            printf("null\n");
        }
        break;
      case 8:
        toSearch = (int*) malloc(sizeof(int));
        printf("At which index delete the element?...\n");
        scanf("%d", toSearch);
        printf("%d\n ", deleteAtIndex(l, *((int *) toSearch), delete_int));
        break;
      case 9:
        toSearch = (int*) malloc(sizeof(int));
        printf("Which Element to delete?...\n");
        scanf("%d", toSearch);
        printf("%d\n ", removeNode(l, (void *) toSearch, int_eq));
        break;
      case 10:
        printList(l2, toString_int);
        break;
      case 11:
        el = (int*) malloc(sizeof(int));
        printf("Inserisci l'elemento da incodare:\n");
        scanf("%d", el);
        enqueue(l2, (void*) el);
        break;
      case 12:
        printf("swap list\n");
        swap(l, l2);
        printf("lista 1\n");
        printList(l, toString_int);
        printf("lista 2\n");
        printList(l2, toString_int);
        break;
      default:
        printf("I am in the default\n");
        destroyList(l, delete_int);
        destroyList(l2, delete_int);
        if(l == NULL){
            printf("ciao\n");
        }
        exit(0);
    }
  }
}*/