#include "scheduler.h"

#define PRIORITY_HIGH -1
#define PRIORITY_MED 0
#define PRIORITY_LOW 1
#define quantum 1000

const int schedulerList[18] = {-1, 0, -1, 0, -1, -1, 0, 1, -1, 1, 0, -1, 0, 1, 0, -1, -1, -1};

struct ucontext_t schedulerContext;
struct ucontext_t mainContext;
struct ucontext_t *activeContext = NULL;

struct Process *highQhead = NULL; //extern
struct Process *highQtail = NULL;
struct Process *medQhead = NULL;
struct Process *medQtail = NULL;
struct Process *lowQhead = NULL; 
struct Process *lowQtail = NULL;

// struct Process *readyQhead = NULL; 
// struct Process *readyQtail = NULL;
struct Process *blockedQhead = NULL; 
struct Process *blockedQtail = NULL;

void scheduler(void){
    
    printf("Inside scheduler\n");

    static int listPointer = 0;

    // if at the end, restart from the first element
    if(listPointer == 18){
        listPointer = 0;
    }

    int num = schedulerList[listPointer];

    if (activeProcess->pcb->status == RUNNING && !activeProcess->pcb->context.uc_link) {
        // Update the process status to terminated
        changeStatus(activeProcess, 0);
        dequeueProcess();
        activeProcess = NULL;
        activeContext = NULL;
    }

    // issue here, check if the head is null for each q, only then add to ready q
    switch (num){
    case PRIORITY_HIGH:
        if (highQhead != NULL){
            activeProcess = highQhead;
            activeContext = &highQhead->pcb->context;
            highQhead->pcb->status = RUNNING;
            highQtail->next = highQhead;
            highQtail = highQhead;
            highQhead = highQhead->next;
        } else {
            printf("empty Q\n");
        }
        
        break;     
    case PRIORITY_LOW:
        if (lowQhead != NULL){
            activeProcess = lowQhead;
            activeContext = &lowQhead->pcb->context;
            lowQhead->pcb->status = RUNNING;
            lowQtail->next = lowQhead;
            lowQtail = lowQhead;
            lowQhead = lowQhead->next;
        } else {
            printf("empty Q\n");
        }
        break;
    
    default:
        if (medQhead != NULL){
            activeProcess = medQhead;
            activeContext = &medQhead->pcb->context;
            medQhead->pcb->status = RUNNING;
            medQtail->next = medQhead;
            medQtail = medQhead;
            medQhead = medQhead->next;
        } else {
            printf("empty Q\n");
        }
        break;
    }

    listPointer++;

    if(activeContext){
        setcontext(activeContext);
    }

    perror("setcontext");
    exit(EXIT_FAILURE);
    
}

void initSchedulerContext(void){
    printf("Inside initSchedulerContext \n");
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

void alarmHandler(int signum) // SIGALRM
{
    printf("inside alarmHandler");
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

void dequeueProcess(void)
{
    while(highQhead->next != NULL){
        struct Process* p = highQhead;
        if(p->next->pcb->status == TERMINATED){
            struct Process* removed = p->next;
            struct Process* newNext = removed->next;
            p->next = newNext;
            removed->next = NULL;
            free(p->pcb->context.uc_stack.ss_sp);
            freeOneProcess(p);
        }
        highQhead = highQhead->next;
    }

    while(medQhead->next != NULL){
        struct Process* p = medQhead;
        if(p->next->pcb->status == TERMINATED){
            struct Process* removed = p->next;
            struct Process* newNext = removed->next;
            p->next = newNext;
            removed->next = NULL;
            free(p->pcb->context.uc_stack.ss_sp);
            freeOneProcess(p);
        }
        medQhead = medQhead->next;
    }

    while(lowQhead->next != NULL){
        struct Process* p = lowQhead;
        if(p->next->pcb->status == TERMINATED){
            struct Process* removed = p->next;
            struct Process* newNext = removed->next;
            p->next = newNext;
            removed->next = NULL;
            free(p->pcb->context.uc_stack.ss_sp);
            freeOneProcess(p);
        }
        lowQhead = lowQhead->next;
    }
}