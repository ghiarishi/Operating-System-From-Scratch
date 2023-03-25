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

#define THREAD_COUNT 4
static ucontext_t mainContext;
static ucontext_t schedulerContext;
static ucontext_t threadContexts[THREAD_COUNT];
static ucontext_t *activeContext;
static const int quantum = 100000; // 10 milliseconds
const int schedulerList[6] = {-1, 0, -1, -1, 0, 1};

// Define the structure for a Process
typedef struct Process{
    struct Process* next; // Pointer to next Process in the queue
    struct processControlBlock* pcb;
} Process;

// Define the structure for a priority queue
typedef struct processQueue{
    Process* head; // Pointer to the first process in the queue
    Process* tail; // Pointer to the last process in the queue
} processQueue;

// Declare the three priority queues
processQueue high_priority_queue = {NULL, NULL};
processQueue medium_priority_queue = {NULL, NULL};
processQueue low_priority_queue = {NULL, NULL};
processQueue readyQueue = {NULL, NULL};
processQueue blockedQueue = {NULL, NULL};
processQueue idleQueue = {NULL, NULL};
// int allProcesses[] = {NULL};

// Function to add a thread to the appropriate priority queue
void enqueue_Process(Process* new_Process) {
    // Determine the appropriate priority queue based on the Processnew_Process's priority level
    processQueue* priority_queue;
    switch(new_Process->pcb->priority) {
        case PRIORITY_HIGH:
            priority_queue = &high_priority_queue;
            break;
        case PRIORITY_MEDIUM:
            priority_queue = &medium_priority_queue;
            break;
        case PRIORITY_LOW:
            priority_queue = &low_priority_queue;
            break;
        default:
            priority_queue = &medium_priority_queue;
            break;
    }
    
    // Add the Processnew_Process to the end of the appropriate priority queue
    if (priority_queue->tail == NULL) {
        priority_queue->head = new_Process;
    } else {
        priority_queue->tail->next = new_Process;
    }
    priority_queue->tail = new_Process;
}

Process* dequeue_Process() {
    // Check if any priority queue is empty
    if (high_priority_queue.head == NULL && medium_priority_queue.head == NULL && low_priority_queue.head == NULL) {
        printf("Error: all queues are empty\n");
        return NULL;
    }

    // Determine the highest priority non-empty queue
    processQueue* priority_queue;
    if (high_priority_queue.head != NULL) {
        priority_queue = &high_priority_queue;
    } else if (medium_priority_queue.head != NULL) {
        priority_queue = &medium_priority_queue;
    } else {
        priority_queue = &low_priority_queue;
    }

    // Remove and return the first thread in the highest priority non-empty queue
    Process* next_Process = priority_queue->head;
    priority_queue->head = next_Process->next;
    if (priority_queue->head == NULL) {
        priority_queue->tail = NULL;
    }
    next_Process->next = NULL;

    // Adjust the priority of the next Process if necessary
    if (next_Process->pcb->priority == PRIORITY_HIGH) {
        next_Process->pcb->priority = PRIORITY_MEDIUM;
    } else if (next_Process->pcb->priority == PRIORITY_MEDIUM) {
        next_Process->pcb->priority = PRIORITY_LOW;
    }

    // Add the next Process back to the appropriate priority queue
    enqueue_Process(next_Process);

    return next_Process;
}

void addtoReadyQ(Process* p){

    processQueue* q = &readyQueue;

    if(q->tail == NULL) {
        q->head = p;
    } else {
        q->tail->next = p;
    }
    q->tail = p;
}

void addtoBlockQ(Process* p){

    processQueue* q = &blockedQueue;

    if(q->tail == NULL) {
        q->head = p;
    } else {
        q->tail->next = p;
    }
    q->tail = p;
}

void addtoIdleQ(Process* p){

    processQueue* q = &idleQueue;

    if(q->tail == NULL) {
        q->head = p;
    } else {
        q->tail->next = p;
    }
    q->tail = p;
}

void newProcess(int id, int priority) {
    // Create a new thread and set its ID and priority level
    ucontext_t *uc0 = malloc(sizeof(ucontext_t));
    Process* new_Process = createPcb(&uc0, id, id, priority, "sleep");
    new_Process->pcb->pid = id;
    new_Process->pcb->priority = (priority != PRIORITY_HIGH && priority != PRIORITY_MEDIUM && priority != PRIORITY_LOW) ? PRIORITY_MEDIUM : priority;
    new_Process->next = NULL;
    
    enqueue_Process(new_Process);

}



static void scheduler(void)
{
    // static int thread = 0;
    // thread++;
    // if (thread == THREAD_COUNT) {
    //     thread = 0;
    //     fputs("Thread 0:\n", stderr);
    // }

    // activeContext = &threadContexts[thread];
    // setcontext(activeContext);
    // perror("setcontext");
    // exit(EXIT_FAILURE);
    
    static int listPointer = 0;

    // if at the end, restart from the first element
    if(listPointer == 5){
        listPointer = 0;
    }

    int num = schedulerList[listPointer];
    Process* p;

    switch (num)
    {
    case -1:
        p = high_priority_queue.head;
        addtoReadyQ(p);
        break;     
    case 0:
        p = medium_priority_queue.head;
        addtoReadyQ(p);
        break;
    case 1:
        p = low_priority_queue.head;
        addtoReadyQ(p);
        break;
    
    default:
        break;
    }

    listPointer++;
    activeContext = readyQueue.head->pcb->context;
    setcontext(activeContext);
    perror("setcontext");
    exit(EXIT_FAILURE);
}

static void f(void){
    printf("helllooooo worllllddd");
}

// static void cat(void) {
//     fputs("cat: started\n", stderr);
//     const size_t size = 4096;
//     char buffer[size];
//     for (;;) {
//         const ssize_t n = read(STDIN_FILENO, buffer, size);
//         if (n == 0) // Ctrl-D
//             break;
//         if (n > 0)
//             write(STDOUT_FILENO, buffer, n);
//     }
//     fputs("cat: returning\n", stderr);
// }
// static void inc(int thread) {
//     for (int i = 0; ; i++) {
//         dprintf(STDERR_FILENO, "%*cThread %d: i = %d\n",
//         thread * 20, ' ', thread, i);
//         usleep(thread * centisecond);
//     }
// }

#include <valgrind/valgrind.h>
static void setStack(stack_t *stack) {
    void *sp = malloc(SIGSTKSZ);
    VALGRIND_STACK_REGISTER(sp, sp + SIGSTKSZ);
    *stack = (stack_t) { .ss_sp = sp, .ss_size = SIGSTKSZ };
}

static void makeContext(ucontext_t *ucp, void (*func)(), int thread) {
    getcontext(ucp);
    sigemptyset(&ucp->uc_sigmask);
    setStack(&ucp->uc_stack);
    ucp->uc_link = func == f? &schedulerContext : NULL;
    if (thread > 0)
    makecontext(ucp, func, 1, thread);
    else
    makecontext(ucp, func, 0);
}

static void makeContexts(void) {
    makeContext(&schedulerContext, scheduler, 0);
    // makeContext(threadContexts, cat, 0);
    // for (int i = 1; i < THREAD_COUNT; i++)
    // makeContext(&threadContexts[i], inc, i);
}

static void alarmHandler(int signum) {// SIGALRM 
    swapcontext(activeContext, &schedulerContext);
}

static void setAlarmHandler(void) {
    struct sigaction act;
    act.sa_handler = alarmHandler;
    act.sa_flags = SA_RESTART;
    sigfillset(&act.sa_mask);
    sigaction(SIGALRM, &act, NULL);
}

static void setTimer(void) {
    struct itimerval it;
    it.it_interval = (struct timeval) { .tv_usec = quantum};
    it.it_value = it.it_interval;
    setitimer(ITIMER_REAL, &it, NULL);
}

static void freeStacks(void) {
    free(schedulerContext.uc_stack.ss_sp);
    for (int i = 0; i < THREAD_COUNT; i++)
        free(threadContexts[i].uc_stack.ss_sp);
}

int main(void) {
    signal(SIGINT, SIG_IGN); // Ctrl-C
    signal(SIGQUIT, SIG_IGN); /* Ctrl-\ */
    signal(SIGTSTP, SIG_IGN); // Ctrl-Z

    newProcess(2245, -1);

    makeContexts();
    setAlarmHandler();
    setTimer();
    swapcontext(&mainContext, activeContext);
    freeStacks();
}