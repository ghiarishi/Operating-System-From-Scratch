#pragma once

#include <signal.h> // sigaction, sigemptyset, sigfillset, signal
#include <stdio.h> // dprintf, fputs, perror
#include <stdbool.h> // boolean 
#include <stdlib.h> // malloc, free
#include <sys/time.h> // setitimer
#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext
#include <unistd.h> // read, usleep, write
#include <valgrind/valgrind.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "pcb.h"
#include "kernel.h"
#include "scheduler.h"
#include "user_functions.h"
#include "parser.h"


char* strCopy(char* src, char* dest);
void freeOneJob(struct Process *proc);
void freeAllJobs(struct Process *head);
struct Process *addJob(struct Process *head, struct Process *newProcess);
void enqueue(struct Process* newProcess);
struct Process *removeJob(struct Process *head, int jobNum);
void dequeue(struct Process *proc);
struct Process *getJob(struct Process *head, int jobNum);
int getCurrentJob(struct Process *head);
void changeStatus(struct Process *head, int jobNum, int newStatus);
void changeFGBG(struct Process *head, int jobNum, int newFGBG);
char *statusToStr(int status);
void sig_handler(int signal);
void penn_shredder(char* buffer);
void *penn_shell(int argc, char** argv);