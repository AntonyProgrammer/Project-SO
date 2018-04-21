#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"
#include "disastrOS_semaphore.h"
#include "disastrOS_semdescriptor.h"
#include "pool_allocator.h"

void internal_semPost() {

  //Recupero informazioni dal PCB sul semaforo di interesse del processo chiamante. Otterrò il descrittore del semaforo
  //relativo al processo chiamante che desidera eseguire la Post
  int fd = running -> syscall_args[0];

  //Controllo se il descrittore è presente nella lista dei descrittori dei semafori del processo per poter eseguire la Post
  SemDescriptor* sem_d = (SemDescriptor*)SemDescriptorList_byFd(&running -> sem_descriptors, fd);

  //Se il descrittore non è nella lista dei descrittori => il semaforo non è presente
  if (!sem_d) {
    running -> syscall_retvalue = DSOS_ESEMNOTPRESENT;
    return;
  }

  //Se il descrittore è nella lista => il semaforo è presente. Lo recupero
  Semaphore* sem = sem_d -> semaphore;

  (sem -> count)++;

  //Nel caso in cui (sem -> count <= 0) è possibile risvegliare un processo dalla waiting_list e modificare il suo stato in Ready
  if (sem -> count <= 0) {
      //NB: nel caso in cui (sem -> count <= 0) c'è sempre un descrittore nella lista dei descrittori in attesa. Lo rimuovo dalla lista...
      SemDescriptorPtr* waiting_descriptor = (SemDescriptorPtr*)List_detach(&sem -> waiting_descriptors, (ListItem*)sem -> waiting_descriptors.first);
      //...e recupero il suo PCB
      PCB* pcb_descriptor = waiting_descriptor -> descriptor -> pcb;
      //Rimuovo il processo dalla lista dei processi in attesa del sistema...
      List_detach(&waiting_list, (ListItem*)pcb_descriptor);
      //...e lo aggiungo nella lista dei processi in esecuzione del sistema stesso
      List_insert(&ready_list, ready_list.last, (ListItem*)pcb_descriptor);
      pcb_descriptor -> status = Ready;
  }

  running -> syscall_retvalue = Success;
  return;

}
