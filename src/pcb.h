#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext
#include <stdio.h>
#include <stdlib.h>

#define RUNNING 2
#define READY 1
#define TERMINATED 0

typedef struct pcb {
    ucontext_t context;
    int pid;
    int priority;
    int ppid;
    char *argument;
    int status;
    // int osStatus;
    // int w_status;
    // int block_time;
}pcb_t;

pcb_t* createPcb(ucontext_t context, int pid, int ppid, int priority, char* argument){
    pcb_t* pcb_obj = (pcb_t*) malloc(sizeof(pcb_t));
    pcb_obj->argument = argument;
    pcb_obj->priority = priority;
    pcb_obj->ppid = ppid;
    pcb_obj->pid = pid;
    pcb_obj->context = context;
    return pcb_obj;
}

void freePcb(struct pcb* pcb_obj){
    free(pcb_obj);
}