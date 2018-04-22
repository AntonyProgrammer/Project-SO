#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include "disastrOS.h"
#include "disastrOS_constants.h"

#define EXECUTIONS 3

void produce(int producer, int consumer) {
  for (int i = 0; i < EXECUTIONS; i++) {
    disastrOS_semWait(producer);
    printf("..........PRODUZIONE..........\n");
    disastrOS_semPost(consumer);
  }
}

void consume(int producer, int consumer) {
  for (int i = 0; i < EXECUTIONS; i++){
    disastrOS_semWait(consumer);
    printf("..........CONSUMO..........\n");
    disastrOS_semPost(producer);
    }
}

// we need this to handle the sleep state
void sleeperFunction(void* args){
  printf("Hello, I am the sleeper, and I sleep %d\n", disastrOS_getpid());
  while(1) {
    getc(stdin);
    disastrOS_printStatus();
  }
}

void childFunction(void* args){
  printf("Hello, I am the child function %d\n", disastrOS_getpid());
  printf("I will iterate a bit, before terminating\n");
  int type=0;
  int mode=0;
  int fd=disastrOS_openResource(disastrOS_getpid(), type, mode);
  printf("fd=%d\n", fd);

  printf("..........APRO I SEMAFORI..........\n");

  int producer = disastrOS_semOpen(1,3);

  int consumer = disastrOS_semOpen(2,0);

  printf("..........SLEEP..........\n");
  disastrOS_sleep(30);


  if (disastrOS_getpid() == 2) {
    printf("..........IL PROCESSO #2 PRODUCE..........\n");
    produce(producer, consumer);
  }

  if (disastrOS_getpid() == 3) {
    printf("..........IL PROCESSO #3 CONSUMA..........\n");
    consume(producer, consumer);
  }
  printf("PID: %d, terminating\n", disastrOS_getpid());

  printf("..........CHIUDO I SEMAFORI..........\n");
  disastrOS_semClose(producer);
  disastrOS_semClose(consumer);
  disastrOS_exit(disastrOS_getpid()+1);
}


void initFunction(void* args) {
  disastrOS_printStatus();
  printf("hello, I am init and I just started\n");
  disastrOS_spawn(sleeperFunction, 0);
  printf("I feel like to spawn 3 nice threads\n");
  int alive_children=0;

  for (int i=0; i<3; ++i) {
    int type=0;
    int mode=DSOS_CREATE;
    printf("mode: %d\n", mode);
    printf("opening resource (and creating if necessary)\n");
    int fd=disastrOS_openResource(i,type,mode);
    printf("fd=%d\n", fd);
    disastrOS_spawn(childFunction, 0);
    alive_children++;
  }

  disastrOS_printStatus();
  int retval;
  int pid;
  while(alive_children>0 && (pid=disastrOS_wait(0, &retval))>=0){
   // disastrOS_printStatus();
    //printf("initFunction, child: %d terminated, retval:%d, alive: %d \n",
	  // pid, retval, alive_children);
    --alive_children;
  }
  disastrOS_printStatus();
  printf("shutdown!\n");
  disastrOS_shutdown();
}

int main(int argc, char** argv){
  char* logfilename=0;
  if (argc>1) {
    logfilename=argv[1];
  }
  // we create the init process processes
  // the first is in the running variable
  // the others are in the ready queue
  printf("the function pointer is: %p", childFunction);
  // spawn an init process
  printf("start\n");
  disastrOS_start(initFunction, 0, logfilename);
  return 0;
}
