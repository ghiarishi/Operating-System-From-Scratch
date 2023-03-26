#include "scheduler.h"

#define PRIORITY_HIGH -1
#define PRIORITY_MED 0
#define PRIORITY_LOW 1
#define quantum 10000

const int schedulerList[18] = {-1, 0, -1, 0, -1, -1, 0, 1, -1, 1, 0, -1, 0, 1, 0, -1, -1, -1};

struct Process *highQhead = NULL; //extern
struct Process *highQtail = NULL;
struct Process *medQhead = NULL;
struct Process *medQtail = NULL;
struct Process *lowQhead = NULL; 
struct Process *lowQtail = NULL;

struct Process *readyQhead = NULL; 
struct Process *readyQtail = NULL;
struct Process *blockedQhead = NULL; 
struct Process *blockedQtail = NULL;

// Function to add a thread to the appropriate priority queue
void enqueueProcess(struct Process* newProcess) {
    
    // Determine the appropriate priority queue based on the ProcessnewProcess's priority level
    switch(newProcess->pcb->priority) {
        case PRIORITY_HIGH:
            if(highQhead == NULL){
                highQhead = newProcess;
            }
            else{
                highQtail->next = newProcess;
            }    
            highQtail = newProcess;
            break;
        case PRIORITY_LOW:
            if(lowQhead == NULL){
                lowQhead = newProcess;
            }
            else{
                lowQtail->next = newProcess;
            }   
            lowQtail = newProcess;
            break;
        default:
            if(medQhead == NULL){
                medQhead = newProcess;
            }
            else{
                medQtail->next = newProcess;
            }   
            medQtail = newProcess;
            break;
    }
}

void testFunc2(){
    printf("helllo world");
}

static void setStack(stack_t *stack){
    void *sp = malloc(SIGSTKSZ);
    // Needed to avoid valgrind errors
    VALGRIND_STACK_REGISTER(sp, sp + SIGSTKSZ);

    *stack = (stack_t){.ss_sp = sp, .ss_size = SIGSTKSZ};
}

struct Process* createNewProcess(void (*func)(), char* argv[], int id, int priority) {
    // Create a new thread and set its ID and priority level

    ucontext_t uc;
    getcontext(&uc);
    sigemptyset(&uc->uc_sigmask);

    printf("before setstack");
    setStack(&uc->uc_stack);
    uc->uc_link = &schedulerContext;
    
    makeContext(&uc, func, 0);
    
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

void addtoReadyQ(struct Process* p){

    // processQueue* q = readyQueue;

    if(readyQhead == NULL) {
        readyQhead = p;
    } 
    else {
        readyQtail->next = p;
    } 
    readyQtail = p;
}

// _______________________________________________________________________________________

// static void scheduler(void){
    
//     static int listPointer = 0;

//     // if at the end, restart from the first element
//     if(listPointer == 5){
//         listPointer = 0;
//     }

//     int num = schedulerList[listPointer];
//     struct Process* p;

//     switch (num)
//     {
//     case PRIORITY_HIGH:
//         p = highQhead;
//         addtoReadyQ(p);
//         break;     
//     case PRIORITY_LOW:
//         p = lowQhead;
//         addtoReadyQ(p);
//         break;
    
//     default:
//         p = medQhead;
//         addtoReadyQ(p);
//         break;
//     }

//     listPointer++;
//     activeContext = readyQhead->pcb->context;
//     setcontext(activeContext);
//     perror("setcontext");
//     exit(EXIT_FAILURE);
// }

// static void alarmHandler(int signum) // SIGALRM
// {
//     swapcontext(activeContext, &schedulerContext);
// }

// static void setAlarmHandler(void)
// {
//     struct sigaction act;

//     act.sa_handler = alarmHandler;
//     act.sa_flags = SA_RESTART;
//     sigfillset(&act.sa_mask);

//     sigaction(SIGALRM, &act, NULL);
// }

// static void setTimer(void)
// {
//     struct itimerval it;

//     it.it_interval = (struct timeval){.tv_usec = quantum * 10};
//     it.it_value = it.it_interval;

//     setitimer(ITIMER_REAL, &it, NULL);
// }

// static void freeStacks(void)
// {
//     free(schedulerContext.uc_stack.ss_sp);
// }

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