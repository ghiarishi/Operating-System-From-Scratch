#pragma once

#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext
#include "pcb.h"
#include "scheduler.h"
#include "shell.h"
#include "user.h"

struct pcb* k_process_create(struct pcb *parent);
// struct Process *p;
void k_process_cleanup(struct pcb *p);