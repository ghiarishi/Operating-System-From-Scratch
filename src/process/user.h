#pragma once

#include <stdio.h>
#include <string.h>
#include <ucontext.h>
#include "scheduler.h"
#include "pcb.h"
#include "kernel.h"
#include "parser.h"

#define MAX_CMD_LENGTH 1000
#define MAX_ARGS 10

extern Process *activeProcess;

#define PROMPT "$ "
#define BUFFERSIZE 4096

char* concat(int argc, char *argv[]); 
pid_t p_spawn(void (*func)(), char *argv[], int fd0, int fd1);

