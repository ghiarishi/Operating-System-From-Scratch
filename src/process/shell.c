#include "shell.h"

#define INPUT_SIZE 4096

// clear all the mallocs to prevent memory leaks
void freeOneProcess(struct Process *proc){
    free(proc->pcb->argument);
    free(proc->pcb);
    free(proc);
}

void changeStatus(struct Process *proc, int newStatus){
    if(newStatus == 2){
        proc -> pcb -> status = RUNNING;
    }
    else if (newStatus == 1){
        proc -> pcb -> status = READY;
    }
    else{
        proc -> pcb -> status = TERMINATED;
    }
}