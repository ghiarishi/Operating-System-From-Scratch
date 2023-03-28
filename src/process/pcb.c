#include "pcb.h"

struct pcb *createPcb(ucontext_t context, int pid, int ppid, int priority, int status, char *argument) {
    struct pcb *pcb_obj = (struct pcb *) malloc(sizeof(struct pcb));
    pcb_obj->argument = argument;
    pcb_obj->priority = priority;
    pcb_obj->ppid = ppid;
    pcb_obj->pid = pid;
    pcb_obj->context = context;
    pcb_obj->status = status;
    return pcb_obj;
}

void freePcb(struct pcb *pcb_obj) {
    free(pcb_obj);
}