#pragma once
#include <stdio.h>
// Define the structure for a Process
typedef struct process{
    struct pcb* pcb;
    struct process* next;
} Process;



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
extern Process *zombieQhead; 
extern Process *zombieQtail;
extern Process *orphanQhead; 
extern Process *orphanQtail;
extern Process *tempHead;
extern Process *tempTail;

extern int ticks; 
extern int fgpid;
static const int quantum = 100000;
// extern FILE *fp;