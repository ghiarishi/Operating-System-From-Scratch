#include "pcb.h"

char* strCopy(char* src, char* dest) {
    strtok(src, "&");
    int i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

struct pcb *createPcb(ucontext_t context, int pid, int pgid, int ppid, int priority, int status, char *input) {
    struct pcb *pcb_obj = (struct pcb *) malloc(sizeof(struct pcb));
    pcb_obj->argument = malloc((strlen(input) + 1) * sizeof(char));
    strCopy(input, pcb_obj->argument);
    pcb_obj->priority = priority;
    pcb_obj->numChild = 1;
    pcb_obj->pid = pid;
    pcb_obj->pgid = pgid;
    pcb_obj->context = context;
    pcb_obj->status = status;
    pcb_obj->pids = malloc(pcb_obj->numChild * sizeof(int));
    pcb_obj->pidsFinished = malloc(pcb_obj->numChild * sizeof(int));
    return pcb_obj;
}

void freePcb(struct pcb *pcb_obj) {
    free(pcb_obj);
}

