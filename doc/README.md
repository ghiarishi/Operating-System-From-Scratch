# Penn - Shell

## Name and PennKey:
Partner 1: Rishi Ghia : ghiar : 24017117
Partner 2: Tanvi Gupta : guptanvi : 45865853
Partner 3: Divya Kumari : divykum2 : 47522818
Partner 4: Andrew Zhy : andrz : 12401358

## List of Submitted Source Files: 

-> root folder
  pennos.c
  pennfat.c
  README.md
  PENN OS COMPANION DOCUMENT
  -> folder common
    errno.c
    errno.h
  -> folder fs
    fat.c
    fat.h
    user.c
    user.h
  -> folder pennfat
    pennfat_utils.c
    pennfat_utils.h
  -> process
    dependencies.c
    dependencies.h
    kernel.c
    kernel.h
    parser.c
    parser.h
    pcb.c
    pcb.h
    scheduler.c
    scheduler.h
    shell.c
    shell.h
    user_functions.c
    user_functions.h
    user.c
    user.h

## Overview of Work Accomplished:

In Project 2 Penn OS, a Unix like operating system was developed using C. It is built on top of the PennShredder and PennShell from previous projects, with the primary addition of the Kernel, Scheduler and PennFat Filesystem. 

### How to run the program: EDIT THIS 
1. Start the Docker
2. In your editor, open Terminal and go to the project's directory using the cd command.
3. Enter the command to open a zsh terminal within Docker: docker exec -it cis 3800 zsh
4. Use the command 'make' to compile the code
5. Commands to run penn-shell: <br>
    * ./penn-shell : This will run the penn-shell in an interactive mode where the shell prints a prompt and waits for the user to input a command. <br>
    * ./penn-shell > filename : This runs the penn-shell in a non-interactive mode where it read commands from a script that the user has mentioned. <br>
    * .penn-shell --async :  This runs it in the asynchronous mode where it reaps all background zombies asynchronously as and when they finish executing <br>


## Description of code and code layout

### dependencies.h

```c
#pragma once
#include <stdio.h>
// Define the structure for a Process
typedef struct process{
    struct pcb* pcb;
    struct process* next;
} Process;

// initialize in .c
extern Process *highQhead;
extern Process *highQtail;
extern Process *medQhead;
extern Process *medQtail;
extern Process *lowQhead; 
extern Process *lowQtail;
extern Process *blockedQhead; 
extern Process *blockedQtail;
extern Process *stoppedQhead; 
extern Process *stoppedQtail;
extern Process *zombieQhead; 
extern Process *zombieQtail;
extern Process *orphanQhead; 
extern Process *orphanQtail;
extern Process *tempHead;
extern Process *tempTail;

extern int ticks; 
extern int fgpid;
static const int quantum = 100000;
```
This file serves as a unified place for us to define a lot of global variables such as the Queue heads and tails, the struct for a process and a few other variables like ticks, the foreground pid, and the time quantum. 

### kernel 
```c
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
void k_process_cleanup(Process *p);
int k_process_kill(Process *p, int signal);
Process *findProcessByPid(int pid);
```
Here we defined the 3 primary signals that we used for the scheduler and shell.

`struct pcb* k_process_create(struct pcb *parent)`
Creates a ucontext and runs the setStack function, initializes the pcb to null, then runs createPcb with the arguments of the context, pidCounter, the parent pid, priority (set to 0 by default) and the status (RUNNING). PidCounter is a global variable that is incremented by 1 each time a PCB is created, starting with 1 for the shell, and moving on from there. Finally, returns the created pcb. 

`void k_process_cleanup(Process *p)`
Used to cleanup, essentially clear out a process from any queues it is in to ensure it cannot be run again by the scheduler, or interfere with any other routines. Generally called when a process enters the terminate context, when it has finished execution gracefully. 
- set status to terminated
- set changedStatus flag
- dequeue from ready queues
- if background process: 
  - enqueue to zombie Q
- iterate through the blocked queue
  - if the pid of current process in Q is the same as the parent of the given process, and the parent is waiting on the current child, then move the parent from the blocked queue and move it to the ready queues. 
- if the current process has children, call k_process_kill on all of them with sigterm as the signal

`int k_process_kill(Process *p, int signal)` 
Used to take action on processes based on the signal argument given. Kills for sigterm, stops for sigstop, and continues in foreground or background for sigcont. 
- switch case based on signal
  - case SIGTERM: 
    - if no children
      - if p is blocked: 
        - unblock p
        - mark as zombie if background process
        - unblock the parent and ready it for execution
      - if p is running: 
        - take process out of ready queues
        - mark as zombie if background process
        - unblock the parent and ready it for execution
      - if p is stopped: 
        - take if off the stopped queue
      - mark the status as SIG_TERMINATED
      - set the changedStatus flag (used in the waitpid function to determine whether a process has changed status)
    - if there are children, iterate through them all, and recursively call k_proc_kill(SIGTERM) on them all
  - case SIGSTOP: 
    - if proc is running: 
      - take off ready queues
    - if blocked, unblock
    - enqueue process to stopped either ways
    - if the parent is blocked: 
      - unblock and ready for execution
    - set the status of process to stopped
    - set changedStatus flag
    - set the bgFlag (mark as background process)
    - return 0
  - case SIGCONT: 
    - reset changedStatus flag
    - if the process status is stopped: 
      - dequeue from stopped
      - if the fg process is shell: 
        - this is the bg command case
        - mark as BG process
        - set to running
        - if sleep process, block it
        - else enqueue to ready
      - if fg is not shell, its the upcoming fg process as marked in shell 
        - this is the fg command case
        - set to foreground
        - if sleep, block and set changedStatus to 0 (for waitpid)
        - if not sleep, set to running and enqueue to ready queues
    - if process status is blocked, and sleep time is remaining, set to FG
    - if process status is running, set to foreground

`Process *findProcessByPid(int pid)`
It is capable of iterating through every single queue mentioned above, and finds the process that we are looking for using the pid. 

### parser
```c
/* Penn-Shell Parser
   hanbangw, 21fa    */

#pragma once

#include <stddef.h>
#include <stdbool.h>

/* Here defines all possible parser errors */
// parser encountered an unexpected file input token '<'
#define UNEXPECTED_FILE_INPUT 1

// parser encountered an unexpected file output token '>'
#define UNEXPECTED_FILE_OUTPUT 2

// parser encountered an unexpected pipeline token '|'
#define UNEXPECTED_PIPELINE 3

// parser encountered an unexpected ampersand token '&'
#define UNEXPECTED_AMPERSAND 4

// parser didn't find input filename following '<'
#define EXPECT_INPUT_FILENAME 5

// parser didn't find output filename following '>' or '>>'
#define EXPECT_OUTPUT_FILENAME 6

// parser didn't find any commands or arguments where it expects one
#define EXPECT_COMMANDS 7

/** 
 * struct parsed_command stored all necessary
 * information needed for penn-shell.
 */
struct parsed_command {
    // indicates the command shall be executed in background
    // (ends with an ampersand '&')
    bool is_background;

    // indicates if the stdout_file shall be opened in append mode
    // ignore this value when stdout_file is NULL
    bool is_file_append;

    // filename for redirecting input from
    const char *stdin_file;

    // filename for redirecting output to
    const char *stdout_file;

    // number of commands (pipeline stages)
    size_t num_commands;

    // an array to a list of arguments
    // size of `commands` is `num_commands`
    char **commands[];
};

/**
 * Arguments:
 *   cmd_line: a null-terminated string that is the command line
 *   result:   a non-null pointer to a `struct parsed_command *`
 * 
 * Return value (int):
 *   an error code which can be,
 *       0: parser finished succesfully
 *      -1: parser encountered a system call error
 *     1-7: parser specific error, see error type above
 * 
 * This function will parse the given `cmd_line` and store the parsed information
 * into a `struct parsed_command`. The memory needed for the struct will be allocated by this
 * function, and the pointer to the memory will be stored into the given `*result`.
 *
 * You can directly use the result in system calls. See demo for more information.
 * 
 * If the function returns a successful value (0), a `struct parsed_command` is guareenteed to be
 * allocated and stored in the given `*result`. It is the caller's responsibility to free the given
 * pointer using `free(3)`.
 * 
 * Otherwise, no `struct parsed_command` is allocated and `*result` is unchanged. If a 
 * system call error (-1) is returned, the caller can use `errno(3)` or `perror(3)` to gain more
 * information about the error.
 */
int parse_command(const char *cmd_line, struct parsed_command **result);


/* This is a debugging function used for outputting a parsed command line. */
void print_parsed_command(const struct parsed_command *cmd);
```

Same code as that given for pennshell, without any modifications made. 

### pcb
```c
#pragma once

#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext
#include <stdio.h>
#include <stdlib.h>
#include "user.h"
#include "../fs/user.h"
// #include "dependencies.h"

#define ZOMBIE 5
#define BLOCKED 4
#define STOPPED 3
#define RUNNING 2
#define SIG_TERMINATED 1
#define TERMINATED 0
#define FG 0
#define BG 1

#define MAX_FILES 512

extern int pidCounter;

struct pcb {
    ucontext_t context;
    int jobID;
    int numChild;
    int pid;
    int ppid;
    int waitChild; 
    int priority;
    char *argument;
    int status;
    int bgFlag;
    int *childPids;                      // list of all pids in the job
    int *childPidsFinished;             // boolean array list that checks every pid is finished
    int sleep_time_remaining;
    int changedStatus;
    file_t *fd_table[MAX_FILES];
};

char* strCopy(char* src, char* dest);

struct pcb *initPCB();

struct pcb *createPcb(ucontext_t context, int pid, int ppid, int priority, int status);

void freePcb(struct pcb *pcb_obj);
```

`char* strCopy(char* src, char* dest)`
Straightforward string copy function

`struct pcb *initPCB()`
Used to initialize the PCB for the pennshell. A separate function is used as a few areguments provided are different, and not all internal variables are required to be initialized, as an exception. 

`struct pcb *createPcb(ucontext_t context, int pid, int ppid, int priority, int status)`
Used to create the pcb for all other processes, setting the arguments and all other fields of the PCB as shown above. 

`void freePcb(struct pcb *pcb_obj)`
Frees the memory used by PCB in order to prevent memory leaks. 

### scheduler.h

```c
#pragma once

#include <signal.h> // sigaction, sigemptyset, sigfillset, signal
#include <stdio.h> // dprintf, fputs, perror
#include <stdbool.h> // boolean 
#include <stdlib.h> // malloc, free
#include <sys/time.h> // setitimer
#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext
#include <unistd.h> // read, usleep, write
#include <valgrind/valgrind.h>
#include "pcb.h"
#include "kernel.h"
#include "user.h"
#include "shell.h"
#include "user_functions.h"
#include "dependencies.h"

#define PRIORITY_HIGH -1
#define PRIORITY_MED 0
#define PRIORITY_LOW 1

extern ucontext_t schedulerContext;
extern ucontext_t *activeContext;
extern ucontext_t idleContext;
extern ucontext_t terminateContext;

void terminateProcess(void);
void scheduler(void);
void initContext(void);
void enqueueBlocked(Process* newProcess);
void enqueueStopped(Process* newProcess);
void enqueue(Process* newProcess);
void enqueueZombie(Process* newProcess);
void dequeueZombie(Process* newProcess);
void dequeueBlocked(Process* newProcess);
void dequeueStopped(Process* newProcess);
void dequeue(Process* newProcess);
void iterateQueue(Process *head);
void alarmHandler(int signum);
void setTimer(void);
void freeStacks(struct pcb *p);
```
Here, we have initialized all the queues to null to start with. Each queue has a head and a tail. We have also got a list of scheduler priorities, which we iterate through incrementally with each pass of the scheduler, scheduling according to the priorities stated. The -1 priority appears 9 times, zero appears 6 times, and 1 appears 3 times. The scheduler uses a round robin system with a time quantum of 100ms to regularly schedule processes according to their respective priorities. 

`void terminateProcess(void)`
This is activated when terminate context is activated, which is done when a process gracefully completes execution. K_process_cleanup is called on the active process, followed by setting the context back to the scheduler to resume scheduling. 

`void scheduler(void)`
Can be thought of as the "main" function here. 
- set list pointer of priorities to 0 right at the beginning, and loop back to 0 once 18 is reached. 
- switch case based on the priority that has been picked from the list of priorities: 
  - if -1: 
    - active context and process set to the head of the High Q. 
    - active process once selected, moved to the back of the queue
    - emptyQflag reset to 0 (used to identify when idle process needs to run)
    - list pointer incremented, to move to next priority
    - activeContext now set as the current context, moving in towards executing the process 
  - if 0: 
    - same process as above
  - if 1: 
    - same process as above
  - if all ready Qs are empty, and the blocked Q head is not null (pennShell), then empty Q flag is set, and idleContext set to the active context
  - If all queues, including blocked q are null, then we just return and terminate the scheduler. 
  - increment list pointer
  - set context to schedulerContext in order to loop through the scheduler again

`void enqueueBlocked(Process* newProcess)`
- sets the status of the process to be enqueued to blocked
- sets the changedStatus flag
- enqueues the process

`void enqueueStopped(Process* newProcess)`
- same as above, but status set to Stopped

`void enqueueZombie(Process* newProcess)`
- same as above, but no status change done

`void enqueue(Process* newProcess)`
- sets the status of the process to running
- switch case between the 3 priorities, enqueuing to the correct queue (READY QUEUES)

`void dequeueZombie(Process* newProcess)`
- iterates through the zombie queue, dequeuing the process asked for

`void dequeueBlocked(Process* newProcess)`
- iterates through the blocked queue, dequeuing the process asked for

`void dequeueStopped(Process* newProcess)`
- iterates through the stopped queue, dequeuing the process asked for

`void dequeue(Process* newProcess)`
- switch case between the 3 priorities, dequeuing from the correct queue (READY QUEUES)

`void iterateQueue(Process *head)`
Testing oriented function used to take as input the head of the queue in question, and print the arguments of each and every process present in that queue

`void initContext(void)`
initializes and sets the scheduler, terminate and idle contexts

`void alarmHandler(int signum)`
- activated each time a sigAlarm is thrown, which allows for the time quantum and scheduler system to work
- iterates through every process in teh blocked Q
  - if the sleep time remaining is greater than 0, and the process is running/blocked, time is decremented by 1 tick
  - if the sleep time remaining is zero, an array is used to store the pid of the process that is completed
- we iterate through the aforementioned array, unblock all the processes, and enqueue them back into the ready queues for cleanup
- swap to the scheduler context to run again

### shell.h
```c
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
#include "dependencies.h"
#include "user_functions.h"
#include "parser.h"
#include "user.h"

#define INPUT_SIZE 4096

#define STOPPED 3
#define RUNNING 2
#define SIG_TERMINATED 1
#define TERMINATED 0
#define FG 0
#define BG 1
#define TRUE 1
#define FALSE 0

#define S_SIGTERM 1
#define S_SIGSTOP 2
#define S_SIGCONT 3

extern int IS_BG;

struct Job{
    int myPid;                      // job ID
    int JobNumber;                  // Counter for current job number since first job begins from 1
    int bgFlag;                     // FG = 0 and BG = 1
    struct Job *next;               // pointer to next job
    char *commandInput;
    int status;                     // tell whether its running or stopped
    int *pids;
    int numChild;
    int *pids_finished;             // boolean array list that checks every pid is finished
};

void setTimer(void);
void signalHandler(int signal);
void sigIntTermHandler(int signal);
void sigcontHandler(int signal);
void sigtstpHandler(int signal);
void setSignalHandler(void);
void pennShredder(char* buffer);
void pennShell();
struct Job *createJob(int pid, int bgFlag, int numChildren, char *input);
struct Job *addJob(struct Job *head, struct Job *newJob);
struct Job *removeJob(struct Job *head, int jobNum);
struct Job *getJob(struct Job *head, int jobNum);
int getCurrentJob(struct Job *head);
void changeStatus(struct Job *head, int jobNum, int newStatus);
void changeFGBG(struct Job *head, int jobNum, int newFGBG);
void iterateShell(struct Job *head);
```
The shell file, which allows for the user to interact with the operating system, along with providing a place for the OS to output various updates. 

Not much being said here, as majority of the functionality is the EXACT same as the pennShell submitted by group 52, where an extremely detailed readme was provided. 

A lot of functions had to be slightly modified in order to integrate with the kernel and scheduler, while maintaining the correct abstraction as required. 

- killpg replaced by p_kill
- fork and execvp replaced by p_spawn
- waitpid replaced by p_waitpid
- all signals replaced with their S_SIGNALNAME counterparts
- WIFSTOPPED, WIFEXITED, WIFSIGNALLED replaced by W_WIFSTOPPED, W_WIFEXITED, W_WIFSIGNALLED 
- sigint, sigterm, sigcont, and sigstop handlers added, essentially each calling p_kill as required

`void setSignalHandler(void)`
Sets and instantiates all signals mentioned above

```c
struct Job *createJob(int pid, int bgFlag, int numChildren, char *input);
struct Job *addJob(struct Job *head, struct Job *newJob);
struct Job *removeJob(struct Job *head, int jobNum);
struct Job *getJob(struct Job *head, int jobNum);
int getCurrentJob(struct Job *head);
void changeStatus(struct Job *head, int jobNum, int newStatus);
void changeFGBG(struct Job *head, int jobNum, int newFGBG);
```
The above functions are exactly the same as Penn-Shell

`void pennShredder(char* buffer)`
- called by pennShell function 
- retains essentially the same functionality as the function with the same name in Penn-Shell project. 
- bg and fg implemented using the p_kill command, with the code for fg also calling p_waitpid with nohang set to false as required. 
- as fork and execvp have been replaced, the original parent-child if else structure has been replaced with an if else ladder that calls p_spawn to fork processes and execute them based on the argument given to buffer. 
- p_waitpid called for foreground jobs with nohang set to false. 

`void pennShell()`
- also retains essentially the same functionality as the main function from penn-shell submission of group 52. 
- primary changes mainly in polling, where p_waitpid is called with no hang set to true in order to poll for background processes. The argument for pid is given as -1, to look at all zombie processes, and is thus called in a while loop. Processes reaped are dealt with according to the status returned from p_waitpid. 
- calls pennShredder function

### user_functions.h
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <ucontext.h>
#include "scheduler.h"
#include "dependencies.h"

void echoFunc(int argc, char *argv[]);
void sleepFunc(int argc, char *argv[]);
void busyFunc(void);
void idleFunc();

// ==== filesystem ====
void catFunc(int argc, char **argv);
void lsFunc(int argc, char **argv);
void touchFunc(int argc, char **argv);
void mvFunc(int argc, char **argv);
void cpFunc(int argc, char **argv);
void rmFunc(int argc, char **argv);
void chmodFunc(int argc, char **argv);

void psFunc (int argc, char **argv);
void killFunc (int argc, char **argv);

void man();

void zombify(int argc, char **argv);
void zombie_child();
void orphanify(int argc, char **argv);
void orphan_child();
void niceFunc(char *argv[]);
int nice_pid(char *argv[]);
void logout();
```
p_spawn calls the functions within this file as it contains the primary functionality for most user functions, or serves as the helper function calling a p_name function in user.c. 

Not much to add to explaining these functions over the code itself. All the filesystem based functions have been explained in the companion document. 

- sleepFunc takes as input the arguments of sleep time, converting it to ticks left, and calling p_sleep. 

### user.h
```c
#pragma once

#include <stdio.h>
#include <string.h>
#include <ucontext.h>
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
pid_t p_spawn(void (*func)(), char *argv[], int fd0, int fd1);
pid_t p_waitpid(pid_t pid, int *wstatus, bool nohang);
void p_sleep(unsigned int ticks1);
int p_kill(pid_t pid, int sig);
void p_exit(void);
int p_nice(pid_t pid, int priority);
```

`char* concat(int argc, char *argv[])`
Used to concatenate strings with each other, used to parse through the arguments array given to pspawn, combining them into just one string that can be given as input to parser.c

`pid_t p_spawn(void (*func)(), char *argv[], int fd0, int fd1)`
- Takes as input the function to be pspawned, the arguments in the form of an array of strings, and the input and output file descriptors. 
- initializes a new pid variable
- mallocs a new process
- calls k_process_create to initialize the pcb
- sets the new pid 
- counts number of arguments, concatenates them, and calls parser.c in order to parse through all arguments, creating a *cmd struct. 
- if the input function is pennshell, sets priority to -1, and calls make context
- if the input function is anything else, starts counting the number of children of the parent, and makes context
- enqueues the process to the ready queues, and returns the pid of the process that was p_spawned

`pid_t p_waitpid(pid_t pid, int *wstatus, bool nohang)`
A brand new implementation of waitpid, done by us. 
- takes as input the pid of the process to be waited on, a pointer to the status variable to store the new status of the process once its reaped, and the nohang variable which indicates the type of wait we must perform. 
- initialized the pid to be returned to 0
- if a no hanging wait (background process):
  - if pid inputted as -1, means we must wait on all processes in the zombie queue. 
    - dequeue the head of the zombie queue
    - store its status
    - set the changed status variable to 0, as the status change has now been taken note of
    - iterate through all the children of the parent process of the current process (if any), and in the childPids array, set it to 0 if the particular field to -2 if the child being waiting on currently matches the field. This is done to ensure we are not waiting on the same process multiple times. 
    - return the pid
  - if pid is given as input
    - iterate through the zombie queue until the process of interest is found, and wait on it using the same technique as shown above. 
    - return the pid
- if a hanging wait (foreground process): 
  - find the process we are to wait on using the pid
  - if the process has changed status (check using the changedStatus flag): 
    - store the pid
    - reap the status
    - reset changedStatus variable to 0
    - iterate through all the children of the parent process of the current process (if any), and in the childPids array, set it to 0 if the particular field to -2 if the child being waiting on currently matches the field. This is done to ensure we are not waiting on the same process multiple times. 
    - return the pid
  - if process hasnt changeds status
    - this means it is either a new process, or is being waited on newly after a past status change was already reaped
    - set the waitchild of the parent process to this process' pid to denote the process the parent is currently waiting for. This ensures that only one foreground process can be waited for at a time. 
    - dequeue the shell from the ready queue, and block it, as this is a blocking wait
    - swap context to the scheduler context in order to resume normal scheduling
    - once the scheduler returns back to this function, it will resume from the perviously stored context
    - if the process has now changed status in this time: 
      - store the status
      - set the changed status variable back to 0
      - iterate through all the children of the parent process of the current process (if any), and in the childPids array, set it to 0 if the particular field to -2 if the child being waiting on currently matches the field. This is done to ensure we are not waiting on the same process multiple times. 
      - return the pid
- return -1 if at any point the wait fails, or there are no more processes left to wait on. 

`int p_kill(pid_t pid, int sig)`
- Used as a helper function to call k_process_kill based on the signal given as input. 

`void p_sleep(unsigned int ticks1)`
- dequeues the active process, which is the shell
- sets the time remaining to the input argument of ticks, which is going to be equal to the amount remaining from any previous executions taken into account
- enqueues the shell into the blocked queue
- swaps context back to the scheduler
- if the time remaning is still greater than 0 once we return from the scheduler, we dequeue the shell from the ready queues again and block it once more

`int p_nice(pid_t pid, int priority)`
- FInd the process in question using the pid
- dequeues the process from the ready queue it is in, sets the new priority according to the argument, and re-enqueues to the newly relevant ready queue. 

# Filesystem Working Notes

Contributors: Andrew Zhu <!-- add yourself here -->

File layout:

- src/
  - pennfat.c - entrypoint for the pennfat standalone
  - fs/
    - fat.c/.h - kernel-level fat functions, file struct defs
    - user.c/.h - user-level fat functions

## Namespacing

All kernel-level functions will start with the prefix `fs_`.

All user-level functions will start with the prefix `f_`.

## Error Codes

PennOS specific filesystem errors will be in the 1000-2000 range.

```c
#define PEHOSTFS 1001  // could not open/close file in host filesystem
#define PEHOSTIO 1002  // could not perform I/O in host filesystem
#define PEBADFS 1003   // invalid PennFAT file, or was otherwise unable to mount
#define PENOFILE 1004  // specified file does not exist
#define PEINUSE 1005   // the specified file is in use in another context and an exclusive operation was called
#define PETOOFAT 1006  // the filesystem is too fat and has no space for a new file
#define PEFMODE 1007   // attempted operation on a file in the wrong mode
#define PEFPERM 1008   // attempted operation on a file without read/write permissions
#define PEFNAME 1009   // the filename is invalid

#define PETOOMANYF 1101 // you have too many files open already
#define PESTDIO 1102    // tried to read from stdout or write to stdin
```

## Structs

When a file is opened, it returns a `struct file`:

```c
typedef struct file {
    filestat_t *entry;  // mmaped to file entry in directory
    uint32_t offset;  // current seek position
    int mode;
    uint8_t stdiomode;  // 0 = FAT file, 1 = stdout, 2 = stdin
} file_t;
```

where `filestat_t` is the directory entry defined in the PennOS handout:

```c
typedef struct filestat {
    char name[32];
    uint32_t size;
    uint16_t blockno;
    uint8_t type;
    uint8_t perm;
    time_t mtime;
    uint8_t unused[16];
} filestat_t;
```

The OS should maintain a file descriptor table for each process linking an int to one of these `struct file`s.
The low-level filesystem implementation operates using pointers to open file structs.

## stdin/stdout

Each `file_t *` struct contains information on whether it is a special file for reading from/writing to stdin/stdout.
The user-level functions `f_read()` and `f_write()` should check this information and redirect to the C API where
necessary rather than the FAT API.

Each process, on creation, should set entries `PSTDIN_FILENO` and `PSTDOUT_FILENO` in its PCB's fd table to file structs
with the correct flag set. This allows for later redirecting stdin/stdout by overwriting the entries in the fd table
with file structs linked to files on the FAT filesystem.

## File Locking Mechanism

In order to prevent multiple writers/conflicting read-writes, the PennFAT filesystem grants exclusive access to any
process that opens a file in a writing mode, and shared access to processes opening a file in read mode. To accomplish
this, the filesystem keeps a record of what files have been opened and in what mode; if a call to `fs_open()` would
violate the locking semantics, the syscall fails with an error.

This record is a `file_t *` array that utilizes array doubling to grow dynamically (initially sized at 4).

## Standalone

The standalone completes the demo plan with no "definitely/indirectly/possibly lost" memory leaks.

## Syscalls

### int f_open(const char *fname, int mode)

Open a file. If the file is opened in F_WRITE or F_APPEND mode, the file is created if it does not exist.

**Parameters**
- `name`: the name of the file to open
- `mode`: the mode to open the file in (F_WRITE, F_READ, or F_APPEND).

**Returns**
the file descriptor of the opened file, -1 on error

**Exceptions**
- `PENOFILE`: the requested file was in read mode and does not exist
- `PEHOSTIO`: failed to read from/write to host filesystem
- `PETOOFAT`: the operation would make a new file but the filesystem is full
- `PEFNAME`: the operation would make a new file but the filename is invalid
- `PEINUSE`: the requested file was opened in an exclusive mode and is currently in use

### int f_close(int fd)

Closes the specified file, freeing any associated memory.

**Parameters**
- `fd`: the file to close

**Returns**
0 on a success, -1 on error.

**Exceptions**
- `PEINVAL`: the file descriptor is invalid

### ssize_t f_read(int fd, int n, char *buf)

Read up to `n` bytes from the specified file into `buf`.

**Parameters**
- `fd`: the file to read from
- `n`: the maximum number of bytes to read
- `buf`: a buffer to store the read bytes

**Returns**
the number of bytes read; -1 on error

**Exceptions**
- `PEFPERM`: you do not have permission to read this file
- `PEHOSTIO`: failed to read from host filesystem

### ssize_t f_write(int fd, const char *str, ssize_t n)

Write up to `n` bytes from `buf` into the specified file.

**Parameters**
- `fd`: the file to write to
- `str`: a buffer storing the bytes to write
- `b`: the maximum number of bytes to write

**Returns**
the number of bytes written; -1 on error

**Exceptions**
- `PEFMODE`: the file is not in write or append mode
- `PEFPERM`: you do not have permission to write to this file
- `PEHOSTIO`: failed to read from host filesystem
- `PETOOFAT`: filesystem is full

### int f_unlink(const char *fname)

Removes the file with the given name.

**Parameters**
- `fname`: the name of the file to delete

**Returns**
0 on success; -1 on error

**Exceptions**
- `PENOFILE`: the specified file does not exist
- `PEHOSTIO`: failed to i/o with the host entry

### uint32_t f_lseek(int fd, int offset, int whence)

Seek the file offset to the given position, given by `offset`. If `whence` is `F_SEEK_SET` this is relative to the 
start of the file, for `F_SEEK_CUR` relative to the current position, and for `F_SEEK_END` relative to the end of the 
file.

**Parameters**
- `fd`: the file to seek
- `offset`: where to seek to relative to `whence`
- `whence`: the seek mode

**Returns**
the new location in bytes from start of file; -1 on error

**Exceptions**
- `PEINVAL`: whence is not a valid option

### filestat_t **f_ls(const char *fname)

Gets information for a file. If `fname` is NULL, gets information for all the files.
It is the caller's responsibility to free each of the returned structs. Use the convenience function `f_freels()` to
do this quickly.

**Parameters**
- `fname`: the name of the file to get the stat of, or NULL to list all files

**Returns**
a pointer to an array of filestat struct pointers. The array will always be terminated with a NULL pointer.

**Exceptions**
- `PEHOSTIO`: failed to read from host filesystem

### void f_freels(filestat_t **stat)

Free the filestat list returned by `f_ls()`.

### int f_rename(const char *oldname, const char *newname)

Rename a file.

**Parameters**
- `oldname`: the old name
- `newname`: the new name

**Returns**
0 on success, -1 on failure

**Exceptions**
- `PENOFILE`: the file does not exist
- `PEHOSTIO`: failed to perform IO on host drive
- `PEFNAME`: the new name is invalid

### int f_chmod(int fd, char mode, uint8_t bitset)

Edit the I/O permissions of an open file.

**Parameters**
- `fd`: the file whose permissions to edit
- `mode`: the mode to edit it in; '+', '=', or '-'
- `bitset`: the permission bitset to edit by (a combination of FAT_EXECUTE, FAT_WRITE, and FAT_READ)

**Returns**
0 on success, -1 on error

**Exceptions**
- `PEINVAL`: the mode is invalid

### Kernel-Level Functions

Kernel-level functions are documented inline in src/fs/fat.c.

===========================================================================================================
# SHELL DESCRIPTION FROM GROUP 52 FOR REFERENCE

1. Header files

2. Defined Macros

3. Created global variables

4. String Copy Function: char* strCopy(char* src, char* dest)
- Function to deep copy a string. Rather helpful inc cases where we don't want to mutate the input string. Such as with using Strtok function. 

5. Job Object: struct Job
- It stores the following variables:
    int pgid;                       // job ID
    int JobNumber;                  // Counter for current job number since first job begins from 1
    int bgFlag;                     // FG = 0 and BG = 1
    struct Job *next;               // pointer to next job
    char *commandInput;             // Input command by user (only to be used when printing updated status)
    int status;                     // tell whether its running or stopped
    int numChild;                   // Indicating the number of piped children process in PGID
    int *pids;                      // list of all pids in the job
    int *pids_finished;             // boolean array list that checks every pid is finished
    
6. Create Job Function: struct Job *createJob(int pgid, int bgFlag, int numChildren, char *input)
- Took as arguments the above stated variables, and assigned the variables of the job object to each of them. Then returned the newly created job to the requesting function. 

7. Free One Job Function: void freeOneJob(struct Job *Job)
- It frees all the memory allocated for a particular job which is passed as an argument to the function. Helps us avoid memory leaks. 

8. Free All Jobs Function: void freeAllJobs(struct Job *head)
- Same as the above function, but calling it on ALL jobs. Very helpful when exiting penn shell. 

9. Add Job to Linked List Function: struct Job *addJob(struct Job *head, struct Job *newJob)
- It adds a job to the linked list and takes as an argument the head of the linked list and the new job to be added. It then returns the head pointer of the altered linked list.

10. Remove Job Function: struct Job *removeJob(struct Job *head, int jobNum)
- Used to remove a job from the linked list. This is done very carefully, ensuring that the links present are removed if needed, and the required ones are repaired correctly. Further, freeOneJob function is also called to clear the memory being used. 
 
11. Retreive job of that Job ID function: struct Job *getJob(struct Job *head, int jobNum)
- To retrieve a job whose job number has been given as an argument to the function.

12. Get the most current job (bg or stopped): int getCurrentJob(struct Job *head)
- penn-shell has a notion of the current job. If there are stopped jobs, the current job is the most recently stopped one. Otherwise, it is the most recently created background job. This fucntion returns the job ID of the current job.

13. Change Status of Job Function: void changeStatus(struct Job *head, int jobNum, int newStatus)
- This changes the status of the job to running, stopped or finished based on the requirement by the builtin command given. 

14. Change FG/BG flag of a process: void changeFGBG(struct Job *head, int jobNum, int newFGBG)
- This function changes the bgFlag (BG to FG or vice versa) variable of the job whose job ID has been passed as an argument. 

15. Convert Status of Job to String function: char *statusToStr(int status)
- Returns the status of the job taken as integer, as string. Useful wehn we want to print the status to the terminal. 

16. Signal Handler for SIGINT, SIGTSTP, SIGCHLD: void sig_handler(int signal)
- We use this function to handle SIGINT, SIGTSTP, and SIGCHLD signals. First, we just print out a new line and the prompt. Then, we use if statements to decide what signal we are trying to handle. If it is SIGINT, a ^C has been entered into the terminal. In this case, we use the killpg() command, using SIGKILL as the flag, to kill the process groun (job) that has been specified. If it is a SIGTSTP, then we ahve encountered a ^Z. In this case, we use the kill command with SIGTSTP as the flag in order to stop the process that has been specified by the pid. 

- Finally, we have SIGCHILD. This part is active only when the async flag is high. We first block the SIGCHLD signal, as while we are already handling it, we don't want another call to this function, as that will create a race condition, which is undesirable. Then, we have a print flag, which is used to determine whether or not we want to print any statuses that we end up with after polling. In order to set it, we check if we are inside the parent process or a foreground child process. If we are in the parent, we can freely print any statuses, so the flag is low. Else it is high, as we don't want to print the statuses while we are in the midst of a child running in the foreground. Next, we perform polling, where we use waitpid(-1) to check for any state changes in any background processes. If status is stopped, when we do what is necessary, if the status is finished, then we mark it as finished, and remove the process from the linked list. Once all of the polling is done within the while loop, we can unblock the sigchild mask and return. 

17. Penn Shredder: void penn_shredder(char* buffer)
- This function has the follwing functionality:
  - It parses the input command by calling the parsed_command() method defined in parser.h
  - Checks the parsed input for errors
  - If the input command is bg, it resumes a stopped process in the BG. It checks whether a job ID has been given or not. If given, it resumes a stopped process in the BG, the job with given job ID or the most recently stopped job. If the job is already running or no such job exists, bg throws an error.
  - If the input command is fg, it brings to the foreground the backgorund job whose job ID has been given, otherwise the current job. If the background job is stopped, fg resumes it before bringing it to the foreground. If the job does not exist, fg throws an error.
  - jobs prints to the standard error all background jobs, sorted by job_id in ascending order. If there are no background jobs, it simply returns without throwing an error.
  - It then creates a child process using fork() system call. Within the child process, it checks if a file has been given for input by the user. If so, it redirects the standaer input to the file using dup2() system call.
  - If a file has been given to write the output to, it redirects standard output to that file after opening it in append or overwrite mode.
  - It then creates and redirects read/write ends of pipes to implement pipelining of commands. 
  - The execvp() system call is used to execute the command.
  - Within the parent process that created the child process(es), setpgid() system call is used to make each child process's group independent. 
  - All the file descriptors are used to close all ends of the pipes to avoid leaks.
  - A new job is created for that process and all the PIDs of the processes are stored in a list.
  - In case of a foreground process, we use tcsetpgrp() system call to give terminal control to the child.
  - We then run waitpid(-group_pid, &status, WUNTRACED) for each process in the job.
  - If we find a process that has been stopped through WIFSTOPPED(status), we change it's status to STOPPED and add it to the linked list.
  - We then give terminal control back to the parent by running tcsetpgrp().
  - During this, if there were any jobs that finished in the background, their status is printed.
  - If the input command was a bakground job, we simply add it to the queue (linked list) after executing it. 

18. Main Function: int main(int argc, char** argv)
- This is the function where it all begins. We read input here, initialize the linked lsit, call penn shredder, check for interactive or non interactive mode, etc. 
- We start out by reading the terminal arguments to main. We perform error checking, and also right here decide if we are in async mode or not. 
- We also initilize our signal handlers here in order to ensure that any and all required signals are handled incase they are encountered in the entireity of the program. We also ignore the SIGTTOU, as it does not let jobs suspend properly otherwise. 
- We first start with polling, since we frist want to see if any processes have finished (mainly for MAC). 
- Once we have polled, we check if we are in interactive or non interactive mode. If we are in non interactive, we simply read lines from the given input file, and execute them by calling pennshredder(). 
- If we are in interactive mode, then we have a lot to do: \
  - First, we write our prompt
  - Then we poll again. This is where we catch any background processes that have finished running, and reap them to ensure they don't stay as zombies, and that we can update their status and print it into the terminal. 
  - This polling also involves taking note of all the various finished processes, and we then remove them all from the linked list, also freeing the allocated memory they have used in the process. 
  - Now, we modify the buffer to make sure it is in a form that is usable to us. We check if it is of length one, or just a new line, in which case we head to the next iteration of the while loop, doing nothing. 
  - We then set the last char of the buffer to a null character to avoid memory leaks
  - Then, we implement several conditions to check for the last character of the buffer not being a new line, as that is when ctrl D has been entered. In that case, we want to kill the program. But if there is an input, we just want to reprompt. 
  - We also check right there if the input is just spaces and tabs, in which case we just reprompt. 
  - FInally, we call penn shredder, whos functionality we have already gone over. 
  - We then set teh current job to the head, as we want to now be able to iterate through the linked list, since we have one or more jobs. 
  - Before exiting main (the entire program), we run freeAllJobs, free(buggerSig), and a few other free()s to ensure there are no memory leaks. This is also done when the program exits due to failure or an error anywhere. 
