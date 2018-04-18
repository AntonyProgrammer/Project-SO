#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"
#include "disastrOS_semaphore.h"
#include "disastrOS_semdescriptor.h"
#include "disastrOS_globals.h"
#include "disastrOS_constants.h"

void internal_semOpen() {

  //Recupero informazioni sul semaforo dal PCB del processo
  int sem_id = running -> syscall_args[0];
  int value = running -> syscall_args[1];

  //Controllo se il semaforo è stato già aperto nel processo con relativa allocazione del suo descrittore inerente al processo corrente
  ListHead semaphores_opened = running -> sem_descriptors;
  SemDescriptor* sd = Find_sd(&semaphores_opened, sem_id);

  //Nel caso il controllo sia positivo restituisco fd al processo...
  if (sd) {
    running -> syscall_retvalue = sd -> fd;
    return;
  }

  //...in caso contrario controllo se il semaforo è stato già creato nel sistema. Se non è stato ancora creato, lo creo
  else {
    Semaphore* sem = SemaphoreList_byId(&semaphores_list, sem_id); //semaphores_list: lista dei semafori del sistema definita in disastrOS.c
    if (!sem) {
      sem = Semaphore_alloc(sem_id, value);

      //Errore nell'allocazione del semaforo
      if (!sem) {
        running -> syscall_retvalue = DSOS_ECREATESEM;
        return;
      }

      //Inserisco il semaforo appena creato nella lista dei semafori del sistema
      List_insert(&semaphores_list, semaphores_list.last, (ListItem*)sem);

    }

    //Creo il descrittore relativo al semaforo
    SemDescriptor* sem_d = SemDescriptor_alloc(running -> last_sem_fd, sem, running);

    //Errore nell'allocazione del descrittore
    if (!sem_d) {
      running -> syscall_retvalue = DSOS_ECREATESEMDESCRIPTOR;
      return;
    }

    running -> last_sem_fd++;

    //Creo il puntatore per avere un riferimento al descrittore relativo al semaforo
    SemDescriptorPtr* sem_dptr = SemDescriptorPtr_alloc(sem_d);

    //Errore nell'allocazione del puntatore
    if (!sem_dptr) {
      running -> syscall_retvalue = DSOS_ECREATESEMDESCRIPTORPTR;
      return;
    }

    sem_d -> ptr = sem_dptr;

    //Creo un secondo puntatore al descrittore relativo al semaforo per evitare conflitti tra la lista dei descrittori
    //e la lista dei descrittori in waiting
    SemDescriptorPtr* sem_dwaitptr = SemDescriptorPtr_alloc(sem_d);

    //Errore nell'allocazione del puntatore
    if (!sem_dwaitptr) {
      running -> syscall_retvalue = DSOS_ECREATESEMDESCRIPTORPTR;
      return;
    }

    sem_d -> waiting_ptr = sem_dwaitptr;

    //Aggiungo il descrittore nella lista inerente ai descrittori dei semafori del processo corrente
    List_insert(&running -> sem_descriptors, running -> sem_descriptors.last, (ListItem*)sem_d);

    //Aggiungo il puntatore al descrittore relativo al semaforo nella lista inerente ai descrittori del semaforo creato
    List_insert(&sem -> descriptors, sem -> descriptors.last, (ListItem*)sem_dptr);

    running -> syscall_retvalue = sem_d -> fd;

  }

  return;

}
