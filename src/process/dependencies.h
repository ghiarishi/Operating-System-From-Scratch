#pragma once


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
