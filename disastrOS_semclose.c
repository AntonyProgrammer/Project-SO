#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"
#include "disastrOS_semaphore.h"
#include "disastrOS_semdescriptor.h"
#include "disastrOS_constants.h"
#include "pool_allocator.h"

void internal_semClose(){

  //Recupero informazioni dal PCB sul semaforo che il processo chiamante desidera deallocare. Otterrò il descrittore del semaforo relativo
  //al processo chiamante che desidera chiudere il semaforo
  int fd = running -> syscall_args[0];
  int ret;

  //Controllo se il descrittore è presente nella lista dei descrittori dei semafori del processo
  SemDescriptor* sem_d = SemDescriptorList_byFd(&running -> sem_descriptors, fd);

  //Se il descrittore non è nella lista dei descrittori => il semaforo non è presente
  if (!sem_d) {
    running -> syscall_retvalue = DSOS_ESEMNOTPRESENT;
    return;
  }

  //Se il descrittore è nella lista => il semaforo e i puntatori al descrittore sono presenti. Li recupero
  Semaphore* sem = sem_d -> semaphore;
  SemDescriptorPtr* sem_dptr = sem_d -> ptr;
  SemDescriptorPtr* sem_dwaitptr = sem_d -> waiting_ptr;

  //Elimino il descrittore dalla lista inerente ai descrittori dei semafori del processo corrente e lo dealloco
  sem_d = (SemDescriptor*)List_detach(&running -> sem_descriptors, (ListItem*)sem_d);
  ret = SemDescriptor_free(sem_d);

  //Errore nella deallocazione del descrittore
  if (ret) {
    running -> syscall_retvalue = ret;
    return;
  }

  //Elimino il puntatore al descrittore relativo al semaforo dalla lista inerente ai descrittori del semaforo creato
  sem_dptr = (SemDescriptorPtr*)List_detach(&sem -> descriptors, (ListItem*)sem_dptr);
  ret = SemDescriptorPtr_free(sem_dptr);

  //Errore nella deallocazione del puntatore
  if (ret) {
    running -> syscall_retvalue = ret;
    return;
  }

  //Elimino il secondo puntatore al descrittore relativo al semaforo dalla lista inerente ai descrittori del semaforo creato
  ret = SemDescriptorPtr_free(sem_dwaitptr);
  //Errore nella deallocazione del puntatore
  if (ret) {
    running -> syscall_retvalue = ret;
    return;
  }

  //Se la lista dei descrittori del semaforo è vuota => il semaforo può essere deallocato
  if ((sem -> descriptors).size == 0) {
    sem = (Semaphore*)List_detach(&semaphores_list, (ListItem*)sem);
    ret = Semaphore_free(sem);

    //Errore nella deallocazione del semaforo
    if (ret) {
      running -> syscall_retvalue = ret;
      return;
    }
	}

  running -> syscall_retvalue = Success;
  return;

}
