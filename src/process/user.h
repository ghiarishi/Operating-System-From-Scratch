#pragma once

#include <ucontext.h>
#include "scheduler.h"
#include "pcb.h"
#include "kernel.h"
#include "parser.h"

extern struct Process *activeProcess;

#define PROMPT "$"
#define BUFFERSIZE 4096

pid_t p_spawn(void (*func)(), char *argv[], int fd0, int fd1);
