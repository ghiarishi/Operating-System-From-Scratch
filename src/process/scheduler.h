#pragma once

#include <signal.h> // sigaction, sigemptyset, sigfillset, signal
#include <stdio.h> // dprintf, fputs, perror
#include <stdbool.h> // boolean 
#include <stdlib.h> // malloc, free
#include <sys/time.h> // setitimer
#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext
#include <unistd.h> // read, usleep, write
#include <valgrind/valgrind.h>
#include "pcb.h"

#define PRIORITY_HIGH -1
#define PRIORITY_MED 0
#define PRIORITY_LOW 1

static ucontext_t *schedulerContext;
static ucontext_t *activeContext;

// initialize in .c
extern struct Process *highQhead;
extern struct Process *highQtail;
extern struct Process *medQhead;
extern struct Process *medQtail;
extern struct Process *lowQhead; 
extern struct Process *lowQtail;

// const int schedulerList[6]; // = {-1, 0, -1, -1, 0, 1};

// Define the structure for a Process
struct Process{
    struct pcb* pcb;
    struct Process* next;
};

void enqueueProcess(struct Process* newProcess);
void testFunc2();
static void setStack(stack_t *stack);
struct Process* createNewProcess(void (*func)(), char* argv[], int id, int priority);
struct Process* dequeueProcess(int priority);
void addtoReadyQ(struct Process* p);

// static void scheduler(void);
// static void alarmHandler(int signum);
// static void setAlarmHandler(void);
// static void setTimer(void);
// static void freeStacks(void);


