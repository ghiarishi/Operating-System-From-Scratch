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

struct pcb *createPcb(ucontext_t context, int pid, int ppid, int priority, int status, char *input) {
    struct pcb *pcb_obj = (struct pcb *) malloc(sizeof(struct pcb));
    pcb_obj->argument = malloc((strlen(input) + 1) * sizeof(char));
    strCopy(input, pcb_obj->argument);
    pcb_obj->priority = priority;
    pcb_obj->pid = pid;
    pcb_obj->context = context;
    pcb_obj->status = status;
    return pcb_obj;
}

void freePcb(struct pcb *pcb_obj) {
    free(pcb_obj);
}

