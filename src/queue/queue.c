#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

void Inizializza(ListaDiElem *ListaPtr) {
  /* Lista è la variabile locale che punta alla "testa di lista".
La funzione assegna alla testa di lista" il valore NULL
corrispondente al valore di lista vuota.
NB: Passaggio per indirizzo */
  { *ListaPtr = NULL; }
}

int IsListaVuota(ListaDiElem Lista)
/* Produce il valore true se la lista passata come parametro
è vuota, false in caso contrario, a Lista viene passato
il valore contenuto nella variabile testa di lista.
Lista punta pertanto al primo elemento della lista considerata */
{

  if (Lista == NULL)
    return 1;
  else
    return 0;
}
void InserisciInTesta(ListaDiElem *ListaPtr, TipoElemento Elem) {
  ListaDiElem Punt;
  /* Allocazione Nuovo Elemento ed inizializzazione puntatore*/
  if (!(Punt = (ElemLista *)malloc(sizeof(ElemLista)))) {
    printf("errore allocazione mem InserisciInTesta\n");
    exit(1);
  }
  Punt->Info = Elem;
  Punt->Prox = *ListaPtr;
  *ListaPtr = Punt;
}

int Cancella(ListaDiElem *ListaPtr, TipoElemento Elem) {
  if ((*ListaPtr)->Info.first == Elem.first &&
      (*ListaPtr)->Info.second == Elem.second) { // elemento trovato in testa
    ListaDiElem ptr = *ListaPtr;   // Aggancia il nodo da rimuovere
    *ListaPtr = (*ListaPtr)->Prox; // Sfila il nodo
    free(ptr);                     // Libera il nodo
    return 1;
  } else { // Ricerca e cancella elemento nella lista
    // Come nel caso inserimento usa due puntatori di servizio
    ListaDiElem Precedente = *ListaPtr, Corrente = (*ListaPtr)->Prox;
    // Ripeti il ciclo per trovare la posizione corretta nella lista
    while (Corrente != NULL &&
           (Corrente->Info.first != Elem.first || Corrente->Info.second)) {
      Precedente = Corrente;     // Va avanti al
      Corrente = Corrente->Prox; //...nodo successivo
    }
    /* Cancella il nodo a cui punta Corrente*/
    if (Corrente != NULL) { // Cancella solo se non siamo alla fine lista
      Precedente->Prox = Corrente->Prox;
      free(Corrente);
      return 1;
    }
  }
  return 0;
}
