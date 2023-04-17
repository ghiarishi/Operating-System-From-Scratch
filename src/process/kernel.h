#pragma once

#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext
#include "pcb.h"
#include "scheduler.h"
#include "shell.h"
#include "user.h"
#include "dependencies.h"

struct pcb* k_process_create(struct pcb *parent);
// Process *p;
void k_process_cleanup(Process *p, int signal);
Process *findProcessByPid(int pid);