#pragma once

#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext
#include <stdio.h>
#include <stdlib.h>
// #include "fs/user.h"

#define STOPPED 3
#define RUNNING 2
#define READY 1
#define TERMINATED 0
#define FG 0
#define BG 1

struct pcb {
    ucontext_t context;
    int jobID;
    int pid;
    int priority;
    char *argument;
    int status;
    int bgFlag;
    // file_t *fd_table[MAX_FILES];
    // int w_status;
};

char* strCopy(char* src, char* dest);

struct pcb *createPcb(ucontext_t context, int pid, int ppid, int priority, int status, char *argument);

void freePcb(struct pcb *pcb_obj);
