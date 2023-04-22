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
#include "dependencies.h"

#define PRIORITY_HIGH -1
#define PRIORITY_MED 0
#define PRIORITY_LOW 1

extern ucontext_t schedulerContext;
extern ucontext_t *activeContext;
extern ucontext_t idleContext;
extern ucontext_t terminateContext;

void terminateProcess(void);
void scheduler(void);
void initContext(void);
void enqueueBlocked(Process* newProcess);
void enqueueStopped(Process* newProcess);
void enqueue(Process* newProcess);
void enqueueZombie(Process* newProcess);
void dequeueZombie(Process* newProcess);
void enqueueOrphan(Process* newProcess);
void dequeueOrphan(Process* newProcess);
void dequeueBlocked(Process* newProcess);
void dequeueStopped(Process* newProcess);
void dequeue(Process* newProcess);
void iterateQueue(Process *head);
void alarmHandler();
void setTimer(void);
void freeStacks(struct pcb *p);


