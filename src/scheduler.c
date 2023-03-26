// #include <signal.h> // sigaction, sigemptyset, sigfillset, signal
// #include <stdio.h> // dprintf, fputs, perror
// #include <stdbool.h> // boolean 
// #include <stdlib.h> // malloc, free
// #include <sys/time.h> // setitimer
// #include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext
// #include <unistd.h> // read, usleep, write
// #include "pcb.h"

#include "scheduler.h"

#define PRIORITY_HIGH -1
#define PRIORITY_MED 0
#define PRIORITY_LOW 1

const int schedulerList[6] = {-1, 0, -1, -1, 0, 1};

// Define the structure for a Process
// struct Process{
//     struct pcb* pcb;
//     struct Process* next;
// };

struct Process *highQhead = NULL;
struct Process *highQtail = NULL;
struct Process *medQhead = NULL;
struct Process *medQtail = NULL;
struct Process *lowQhead = NULL; 
struct Process *lowQtail = NULL;

// Function to add a thread to the appropriate priority queue
void enqueueProcess(struct Process* newProcess) {
    
    // Determine the appropriate priority queue based on the ProcessnewProcess's priority level
    switch(newProcess->pcb->priority) {
        case PRIORITY_HIGH:
            if(highQhead == NULL){
                highQhead = newProcess;
                highQtail = newProcess;
            }
            else{
                highQtail->next = newProcess;
                highQtail = newProcess;
            }    
            break;
        case PRIORITY_LOW:
            if(lowQhead == NULL){
                lowQhead = newProcess;
                lowQtail = newProcess;
            }
            else{
                lowQtail->next = newProcess;
                lowQtail = newProcess;
            }   
            break;
        default:
            if(medQhead == NULL){
                medQhead = newProcess;
                medQtail = newProcess;
            }
            else{
                medQtail->next = newProcess;
                medQtail = newProcess;
            }   
            break;
    }
}

struct Process* createNewProcess(int id, int priority) {
    // Create a new thread and set its ID and priority level
    ucontext_t *uc = malloc(sizeof(ucontext_t));
    struct Process *newProcess = malloc(sizeof(struct Process));
    newProcess->pcb = createPcb(*uc, id, id, priority, "sleep 5");
    printf("Creating new process\n");
    printf("PID is %d",newProcess->pcb->pid);
    enqueueProcess(newProcess);
    return newProcess;
}

struct Process* dequeueProcess(int priority) {
    // struct Process *dequeued = malloc(sizeof(struct Process));
    struct Process *dequeued = NULL;
    switch(priority) {
        case PRIORITY_HIGH:
            dequeued = highQhead;
            highQhead = highQhead->next;
            return dequeued;
        case PRIORITY_LOW:
            dequeued = lowQhead;
            lowQhead = lowQhead->next;
            return dequeued;
        default:
            dequeued = medQhead;
            medQhead = medQhead->next;
            return dequeued;
    }
}

// int main(int argc, char** argv) {
//     printf("main\n");

//     signal(SIGINT, SIG_IGN); // Ctrl-C
//     signal(SIGQUIT, SIG_IGN); /* Ctrl-\ */
//     signal(SIGTSTP, SIG_IGN); // Ctrl-Z

//     struct Process* test_process = createNewProcess(500, -1);
//     printf("%s\n", test_process->pcb->argument);
//     free(test_process);
    
//     return 0;
// }