#include <signal.h> // sigaction, sigemptyset, sigfillset, signal
#include <stdio.h> // dprintf, fputs, perror
#include <stdbool.h> // boolean 
#include <stdlib.h> // malloc, free
#include <sys/time.h> // setitimer
#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext
#include <unistd.h> // read, usleep, write
#include "pcb.h"

#define PRIORITY_HIGH -1
#define PRIORITY_MEDIUM 0
#define PRIORITY_LOW 1

// Define the structure for a Process
typedef struct Process{
    struct Process* next; // Pointer to next Process in the queue
    struct pcb* pcb;
} Process;

// Define the structure for a priority queue
typedef struct processQueue{
    Process* head; // Pointer to the first process in the queue
    Process* tail; // Pointer to the last process in the queue
} processQueue;

processQueue *medium_priority_queue = {NULL, NULL};

// Function to add a thread to the appropriate priority queue
void enqueue_Process(Process* new_Process) {
    // Determine the appropriate priority queue based on the Processnew_Process's priority level
    processQueue *priority_queue = NULL; 
    priority_queue = medium_priority_queue;
    priority_queue->head = new_Process;
    
    // Add the Processnew_Process to the end of the appropriate priority queue
    // if (priority_queue->tail == NULL) {
    //     priority_queue->head = new_Process;
    // } else {
    //     priority_queue->tail->next = new_Process;
    // }
    // priority_queue->tail = new_Process;
}

void newProcess(int id, int priority) {
    // Create a new thread and set its ID and priority level
    ucontext_t *uc = malloc(sizeof(ucontext_t));
    // Process* new_Process = NULL;
    Process* new_Process = malloc(sizeof(Process));
    new_Process->pcb = createPcb(uc, id, id, priority, "sleep 5");
    new_Process->next = NULL;
    printf("Hello there");
    // enqueue_Process(new_Process);
}


int main() {
    signal(SIGINT, SIG_IGN); // Ctrl-C
    signal(SIGQUIT, SIG_IGN); /* Ctrl-\ */
    signal(SIGTSTP, SIG_IGN); // Ctrl-Z

    newProcess(500, 0);
    return 0;
}