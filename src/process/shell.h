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
#include "user_functions.h"
#include "parser.h"
#include "user.h"
#include "dependencies.h"
// #include "../pennfat/pennfat_utils.h"

void sigint_handler(int signal);
void pennShell();

// char* strCopy(char* src, char* dest);
void freeOneJob(Process **proc);
void freeAllJobs(Process **head);
void addJob(Process **head, Process **tail, Process *newProcess);
void enqueue(Process* newProcess);
void removeJob(Process **head, int jobNum);
void dequeue(Process *proc);
// Process *getJob(Process *head, int jobNum);
// int getCurrentJob(Process *head);
// void changeStatus(Process *head, int jobNum, int newStatus);
// void changeFGBG(Process *head, int jobNum, int newFGBG);
// char *statusToStr(int status);
// void sig_handler(int signal);
// void penn_shredder(char* buffer);
// void penn_shell(int argc, char** argv);

// input parameters: head of LL, newJob we want to add to LL
