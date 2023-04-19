#pragma once

#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext
#include "pcb.h"
#include "scheduler.h"
#include "shell.h"
#include "user.h"
#include "dependencies.h"

#define S_SIGTERM 1
#define S_SIGSTOP 2
#define S_SIGCONT 3

struct pcb* k_process_create(struct pcb *parent);
// Process *p;
void k_process_cleanup(Process *p);
int k_process_kill(Process *p, int signal);
Process *findProcessByPid(int pid);