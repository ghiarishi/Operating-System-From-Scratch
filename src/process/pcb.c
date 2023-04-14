#include "pcb.h"

struct pcb *initPCB() {
    struct pcb *pcb_obj = (struct pcb *) malloc(sizeof(struct pcb));
    pcb_obj->argument = NULL;
    pcb_obj->priority = -1;
    pcb_obj->numChild = 1;
    pcb_obj->pid = pidCounter;
    pcb_obj->ppid = pidCounter;
    pcb_obj->status = READY;
    pcb_obj->pids = malloc(pcb_obj->numChild * sizeof(int));
    pcb_obj->pidsFinished = malloc(pcb_obj->numChild * sizeof(int));
    return pcb_obj;
}

struct pcb *createPcb(ucontext_t context, int pid, int ppid, int priority, int status) {
    struct pcb *pcb_obj = (struct pcb *) malloc(sizeof(struct pcb));
    pcb_obj->argument = NULL; // malloc((strlen(input) + 1) * sizeof(char));
    pcb_obj->priority = priority;
    pcb_obj->numChild = 1;
    pcb_obj->pid = pid;
    pcb_obj->ppid = ppid;
    pcb_obj->context = context;
    pcb_obj->status = status;
    pcb_obj->pids = malloc(pcb_obj->numChild * sizeof(int));
    pcb_obj->pidsFinished = malloc(pcb_obj->numChild * sizeof(int));
    // pcb_obj -> childArgs = NULL;
    return pcb_obj;
}

void freePcb(struct pcb *pcb_obj) {
    free(pcb_obj);
}

