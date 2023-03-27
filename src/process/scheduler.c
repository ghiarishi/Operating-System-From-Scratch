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

void scheduler(void){
    
    printf("inside scheduler");

    static int listPointer = 0;

    // if at the end, restart from the first element
    if(listPointer == 18){
        listPointer = 0;
    }

    int num = schedulerList[listPointer];
    struct Process* p;

    switch (num)
    {
    case PRIORITY_HIGH:
        p = highQhead;
        addtoReadyQ(p);
        break;     
    case PRIORITY_LOW:
        p = lowQhead;
        addtoReadyQ(p);
        break;
    
    default:
        p = medQhead;
        addtoReadyQ(p);
        break;
    }

    listPointer++;

    activeContext = &readyQhead->pcb->context;
    setcontext(activeContext);
    perror("setcontext");
    exit(EXIT_FAILURE);
}

void initSchedulerContext(void){
    printf("inside initSchedulerContext");
    getcontext(&schedulerContext);
    schedulerContext.uc_stack.ss_sp = malloc(SIGSTKSZ);
    schedulerContext.uc_stack.ss_size = SIGSTKSZ;
    schedulerContext.uc_link = NULL;
    makecontext(&schedulerContext, scheduler, 0);
}

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

void setStack(stack_t *stack){
    void *sp = malloc(SIGSTKSZ);
    // Needed to avoid valgrind errors
    VALGRIND_STACK_REGISTER(sp, sp + SIGSTKSZ);

    *stack = (stack_t){.ss_sp = sp, .ss_size = SIGSTKSZ};
}

struct Process* createNewProcess(void (*func)(), char* argv[], int id, int priority) {
    // Create a new process and set its ID and priority level
    
    ucontext_t *uc = (ucontext_t *) malloc(sizeof(ucontext_t));
    getcontext(uc);
    sigemptyset(&uc->uc_sigmask);

    printf("before setstack");
    setStack(&uc->uc_stack);
    uc->uc_link = &schedulerContext;

    makecontext(uc, func, 3, argv);
    
    struct Process *newProcess = (struct Process*) malloc(sizeof(struct Process));

    newProcess->pcb = createPcb(*uc, id, id, priority, "echo hello world");
    printf("Creating new process\n");
    printf("PID is %d\n", newProcess->pcb->pid);
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

void alarmHandler(int signum) // SIGALRM
{
    swapcontext(activeContext, &schedulerContext);
}

void setAlarmHandler(void)
{
    struct sigaction act;

    act.sa_handler = alarmHandler;
    act.sa_flags = SA_RESTART;
    sigfillset(&act.sa_mask);

    sigaction(SIGALRM, &act, NULL);
}

void setTimer(void)
{
    struct itimerval it;

    it.it_interval = (struct timeval){.tv_usec = quantum};
    it.it_value = it.it_interval;

    setitimer(ITIMER_REAL, &it, NULL);
}

void freeStacks(void)
{
    free(schedulerContext.uc_stack.ss_sp);
}