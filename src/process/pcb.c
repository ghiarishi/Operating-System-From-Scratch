#include "pcb.h"

struct pcb *initPCB() {
    struct pcb *pcb_obj = (struct pcb *) malloc(sizeof(struct pcb));
    pcb_obj->argument = NULL;
    pcb_obj->priority = -1;
    pcb_obj->numChild = 0;
    pcb_obj->pid = pidCounter;
    pcb_obj->ppid = pidCounter;
    pcb_obj->status = READY;
    pcb_obj->childPids = malloc(20 * sizeof(int));
    pcb_obj->childPidsFinished = malloc(20 * sizeof(int));
    pcb_obj->sleep_time_remaining = -1;
    return pcb_obj;
}

struct pcb *createPcb(ucontext_t context, int pid, int ppid, int priority, int status) {
    struct pcb *pcb_obj = (struct pcb *) malloc(sizeof(struct pcb));
    pcb_obj->argument = NULL; // malloc((strlen(input) + 1) * sizeof(char));
    pcb_obj->priority = priority;
    pcb_obj->numChild = 0;
    pcb_obj->pid = pid;
    pcb_obj->ppid = ppid;
    pcb_obj->context = context;
    pcb_obj->status = status;
    pcb_obj->changedStatus = 0;
    pcb_obj->childPids = malloc(20 * sizeof(int));
    pcb_obj->childPidsFinished = malloc(20 * sizeof(int));
    pcb_obj->sleep_time_remaining = -1;
    bzero(pcb_obj->fd_table, sizeof(file_t *) * MAX_FILES);
    file_t *special_stdin_file = malloc(sizeof(file_t));
    special_stdin_file->stdiomode = FIO_STDIN;
    file_t *special_stdout_file = malloc(sizeof(file_t));
    special_stdout_file->stdiomode = FIO_STDOUT;
    pcb_obj->fd_table[PSTDIN_FILENO] = special_stdin_file;
    pcb_obj->fd_table[PSTDOUT_FILENO] = special_stdout_file;
    pcb_obj->bgFlag = IS_BG;

    return pcb_obj;
}

void freePcb(struct pcb *pcb_obj) {
    free(pcb_obj);
}

