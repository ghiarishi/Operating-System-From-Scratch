#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext
#include <stdio.h>
#include <stdlib.h>

#define RUNNING 2
#define READY 1
#define TERMINATED 0

struct pcb {
    ucontext_t context;
    int pid;
    int priority;
    int ppid;
    char *argument;
    int status;
    // int osStatus;
    // int w_status;
    // int block_time;
};

struct pcb* createPcb(ucontext_t context, int pid, int ppid, int priority, char* argument){
    struct pcb * pcb_obj = (struct pcb *) malloc(sizeof(struct pcb));
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