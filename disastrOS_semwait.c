#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"
#include "disastrOS_semaphore.h"
#include "disastrOS_semdescriptor.h"
#include "pool_allocator.h"

void internal_semWait() {

  //Recupero informazioni dal PCB sul semaforo di interesse del processo chiamante. Otterrò il descrittore del semaforo
  //relativo al processo chiamante che desidera eseguire la Wait
  int fd = running -> syscall_args[0];

  //Controllo se il descrittore è presente nella lista dei descrittori dei semafori del processo per poter eseguire la Wait
  SemDescriptor* sem_d = (SemDescriptor*)SemDescriptorList_byFd(&running -> sem_descriptors, fd);

  //Se il descrittore non è nella lista dei descrittori => il semaforo non è presente
  if (!sem_d) {
    running -> syscall_retvalue = DSOS_ESEMNOTPRESENT;
    return;
  }

  //Se il descrittore è nella lista => il semaforo è presente. Lo recupero
  Semaphore* sem = sem_d -> semaphore;

  //Recupero anche il secondo riferimento al descrittore relativo al semaforo
  SemDescriptorPtr* sem_dwaitptr = sem_d -> waiting_ptr;

  (sem -> count)--;

  //Nel caso in cui (sem -> count < 0) lo stato del processo deve essere messo in Waiting
  if (sem -> count < 0) {
    //Aggiungo il descrittore nella lista dei descrittori in attesa
    List_insert(&(sem_d -> semaphore -> waiting_descriptors), sem -> waiting_descriptors.last, (ListItem*)sem_dwaitptr);
    //Aggiungo il processo nella lista dei processi in attesa del sistema
    List_insert(&waiting_list, waiting_list.last, (ListItem*)running);
    running -> status = Waiting;
    //Nella lista dei processi ready del sistema c'è sempre almeno un processo -> sleeper
    running = (PCB*)List_detach(&ready_list, ready_list.first);
  }

  running -> syscall_retvalue = Success;
  return;

}
