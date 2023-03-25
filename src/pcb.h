#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext
#include <stdio.h>
#include <stdlib.h>

typedef struct processControlBlock {
    ucontext_t *context;
    int pid;
    int priority;
    int ppid;
    char *argument;
    int userStatus;
    int osStatus;
    int w_status;
    int block_time;
} processControlBlock;

processControlBlock* createPcb(ucontext_t* context, int pid, int ppid, int priority, char* argument){
    processControlBlock* pcb = ((processControlBlock*) malloc(sizeof(processControlBlock)));
    pcb->argument = argument;
    pcb->priority = priority;
    pcb->ppid = ppid;
    pcb->pid = pid;
    pcb->context = context;
    return pcb;
}

void freePcb(processControlBlock* pcb){
    free(pcb);
}