#pragma once

#include <signal.h> // sigaction, sigemptyset, sigfillset, signal
#include <stdio.h> // dprintf, fputs, perror
#include <stdbool.h> // boolean 
#include <stdlib.h> // malloc, free
#include <sys/time.h> // setitimer
#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext
#include <unistd.h> // read, usleep, write
#include "pcb.h"

#define PRIORITY_HIGH -1
#define PRIORITY_MED 0
#define PRIORITY_LOW 1

// const int schedulerList[6]; // = {-1, 0, -1, -1, 0, 1};

// Define the structure for a Process
struct Process{
    struct pcb* pcb;
    struct Process* next;
};


struct Process* createNewProcess(int id, int priority);

void enqueueProcess(struct Process* newProcess);

struct Process* dequeueProcess(int priority);


