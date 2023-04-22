#pragma once

#include <stdio.h>
#include <string.h>
#include <ucontext.h>
#include <regex.h>
#include "scheduler.h"
#include "pcb.h"
#include "parser.h"
#include "dependencies.h"

#define MAX_CMD_LENGTH 1000
#define MAX_ARGS 10

#define S_SIGTERM 1
#define S_SIGSTOP 2
#define S_SIGCONT 3

extern Process *activeProcess;

#define PROMPT "$ "
#define BUFFERSIZE 4096

char* concat(int argc, char *argv[]); 
void removeNicePrefix(char *argv[], int argc);
pid_t p_spawn(void (*func)(), char *argv[], int fd0, int fd1);
pid_t p_waitpid(pid_t pid, int *wstatus, bool nohang);
void p_sleep(unsigned int ticks1);
int p_kill(pid_t pid, int sig);
void p_exit(void);
int p_nice(pid_t pid, int priority);
