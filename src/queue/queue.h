#ifndef __QUEUE_H_
#define __QUEUE_H_

struct Pair {
  int first;
  int second;
};
typedef struct Pair Pair;

typedef Pair TipoElemento;

struct EL {
	TipoElemento Info;
	struct EL *Prox;
};
typedef struct EL ElemLista;
typedef ElemLista *ListaDiElem;

void Inizializza(ListaDiElem *ListaPtr);
int IsListaVuota (ListaDiElem Lista);
void InserisciInTesta(ListaDiElem *ListaPtr, TipoElemento Elem);
int Cancella(ListaDiElem *ListaPtr, TipoElemento Elem);

#endif
