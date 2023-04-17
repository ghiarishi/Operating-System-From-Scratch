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
// #include "dependencies.h"
// #include "../pennfat/pennfat_utils.h"

// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <string.h>
// #include <sys/wait.h>
// #include <signal.h>
// #include <fcntl.h>
// #include "parser.h"

#define INPUT_SIZE 4096

#define STOPPED 3
#define RUNNING 2
#define READY 1
#define TERMINATED 0
#define FG 0
#define BG 1
#define TRUE 1
#define FALSE 0

char **bufferSig;

int async = 0;
int IS_BG = 0;
int pgid = 0;
int curr_pid = 0;
int par_pgid  = 0;
int bufferWaiting = 0;
int bufferCount;


struct Job{
    int pgid;                       // job ID
    int JobNumber;                  // Counter for current job number since first job begins from 1
    int bgFlag;                     // FG = 0 and BG = 1
    struct Job *next;               // pointer to next job
    char *commandInput;             // Input command by user (only to be used when printing updated status)
    int status;                     // tell whether its running or stopped
    int numChild;                   // Indicating the number of piped children process in PGID
    int *pids;                      // list of all pids in the job
    int *pids_finished;             // boolean array list that checks every pid is finished
};

void setTimer(void);
void sigintHandler(int signal);
void sigtermHandler(int signal);
void sigcontHandler(int signal);
void sigtstpHandler(int signal);
void setSignalHandler(void);
void pennShell();
struct Job *createJob(int pgid, int bgFlag, int numChildren, char *input);
void freeOneJob(struct Job *Job);
void freeAllJobs(struct Job *head);
struct Job *addJob(struct Job *head, struct Job *newJob);
struct Job *removeJob(struct Job *head, int jobNum);
struct Job *getJob(struct Job *head, int jobNum);
int getCurrentJob(struct Job *head);
void changeStatus(struct Job *head, int jobNum, int newStatus);
// int getCurrentJob(Process *head);
// void changeStatus(Process *head, int jobNum, int newStatus);
// void changeFGBG(Process *head, int jobNum, int newFGBG);
// char *statusToStr(int status);
// void sig_handler(int signal);
// void penn_shredder(char* buffer);
// void penn_shell(int argc, char** argv);

// input parameters: head of LL, newJob we want to add to LL
