#include "pcb.h"

int pidCounter = 1;

struct pcb *initPCB() {
    struct pcb *pcb_obj = (struct pcb *) malloc(sizeof(struct pcb));
    // pcb_obj->argument = NULL;
    pcb_obj->priority = -1;
    pcb_obj->numChild = 1;
    pcb_obj->pid = pidCounter;
    pcb_obj->pgid = pidCounter;
    pcb_obj->status = READY;
    pcb_obj->pids = malloc(pcb_obj->numChild * sizeof(int));
    pcb_obj->pidsFinished = malloc(pcb_obj->numChild * sizeof(int));
    // printf("inside initPCB \n");

    // bzero(fd_table, sizeof(file_t *) * MAX_FILES);
    // file_t *special_stdin_file = malloc(sizeof(file_t));
    // special_stdin_file->stdiomode = FIO_STDIN;
    // file_t *special_stdout_file = malloc(sizeof(file_t));
    // special_stdout_file->stdiomode = FIO_STDOUT;
    // fd_table[PSTDIN_FILENO] = special_stdin_file;
    // fd_table[PSTDOUT_FILENO] = special_stdout_file;

    return pcb_obj;
}

struct pcb *createPcb(ucontext_t context, int pid, int pgid, int priority, int status) {
    struct pcb *pcb_obj = (struct pcb *) malloc(sizeof(struct pcb));
    // pcb_obj->argument = malloc((strlen(input) + 1) * sizeof(char));
    // strcpy(input, pcb_obj->argument);
    pcb_obj->priority = priority;
    pcb_obj->numChild = 1;
    pcb_obj->pid = pid;
    pcb_obj->pgid = pgid;
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

