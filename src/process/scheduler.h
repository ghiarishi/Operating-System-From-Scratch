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
#include "kernel.h"
#include "user.h"
#include "shell.h"
#include "user_functions.h"

#define PRIORITY_HIGH -1
#define PRIORITY_MED 0
#define PRIORITY_LOW 1

extern ucontext_t schedulerContext;
extern ucontext_t mainContext;
extern ucontext_t *activeContext;

// initialize in .c
extern struct Process *highQhead;
extern struct Process *highQtail;
extern struct Process *medQhead;
extern struct Process *medQtail;
extern struct Process *lowQhead; 
extern struct Process *lowQtail;

// Define the structure for a Process
struct Process{
    struct pcb* pcb;
    struct Process* next;
};

void scheduler(void);
void initSchedulerContext(void);
void testFunc2();
void setStack(stack_t *stack);
struct Process* createNewProcess(void (*func)(), char* argv[], int id, int priority);
void alarmHandler(int signum);
void setAlarmHandler(void);
void setTimer(void);
void freeStacks(void);


