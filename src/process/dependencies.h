#pragma once

// Define the structure for a Process
typedef struct process{
    struct pcb* pcb;
    struct process* next;
} Process;

#define S_SIGTERM 1
#define S_SIGSTOP 2
#define S_SIGCONT 3
#define S_SIGTSTP 4

// initialize in .c
extern Process *highQhead;
extern Process *highQtail;
extern Process *medQhead;
extern Process *medQtail;
extern Process *lowQhead; 
extern Process *lowQtail;
extern Process *blockedQhead; 
extern Process *blockedQtail;
extern Process *stoppedQhead; 
extern Process *stoppedQtail;

extern int ticks; 
static const int quantum = 100000;