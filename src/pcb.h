#pragma once

#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext
#include <stdio.h>
#include <stdlib.h>
// #include "fs/user.h"

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
    // file_t *fd_table[MAX_FILES];
    // int osStatus;
    // int w_status;
    // int block_time;
};

struct pcb *createPcb(ucontext_t context, int pid, int ppid, int priority, char *argument);
void freePcb(struct pcb *pcb_obj);
