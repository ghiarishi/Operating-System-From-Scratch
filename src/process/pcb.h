#pragma once

#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext
#include <stdio.h>
#include <stdlib.h>
#include "user.h"
#include "../fs/user.h"
// #include "dependencies.h"

#define BLOCKED 4
#define STOPPED 3
#define RUNNING 2
#define SIG_TERMINATED 1
#define TERMINATED 0
#define FG 0
#define BG 1

#define MAX_FILES 512

extern int pidCounter;

struct pcb {
    ucontext_t context;
    int jobID;
    int numChild;
    int pid;
    int ppid;
    int waitChild; 
    int priority;
    char *argument;
    int status;
    int bgFlag;
    int *childPids;                      // list of all pids in the job
    int *childPidsFinished;             // boolean array list that checks every pid is finished
    int sleep_time_remaining;
    int changedStatus;
    file_t *fd_table[MAX_FILES];
};

char* strCopy(char* src, char* dest);

struct pcb *initPCB();

struct pcb *createPcb(ucontext_t context, int pid, int ppid, int priority, int status);

void freePcb(struct pcb *pcb_obj);
