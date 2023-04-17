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
#include "kernel.h"
#include "user_functions.h"
#include "parser.h"
#include "user.h"

#define INPUT_SIZE 4096

#define STOPPED 3
#define RUNNING 2
#define READY 1
#define TERMINATED 0
#define FG 0
#define BG 1
#define TRUE 1
#define FALSE 0

struct Job{
    int myPid;                      // job ID
    int JobNumber;                  // Counter for current job number since first job begins from 1
    int bgFlag;                     // FG = 0 and BG = 1
    struct Job *next;               // pointer to next job
    char *commandInput;             // Input command by user (only to be used when printing updated status)
    int status;                     // tell whether its running or stopped
};

void setTimer(void);
void sigIntTermHandler(int signal);
void sigcontHandler(int signal);
void sigtstpHandler(int signal);
void setSignalHandler(void);
void pennShredder(char* buffer);
void pennShell();
struct Job *createJob(int pid, int bgFlag, int numChildren, char *input);
void freeOneJob(struct Job *Job);
void freeAllJobs(struct Job *head);
struct Job *addJob(struct Job *head, struct Job *newJob);
struct Job *removeJob(struct Job *head, int jobNum);
struct Job *getJob(struct Job *head, int jobNum);
int getCurrentJob(struct Job *head);
void changeStatus(struct Job *head, int jobNum, int newStatus);