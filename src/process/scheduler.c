#include "scheduler.h"

#define PRIORITY_HIGH -1
#define PRIORITY_MED 0
#define PRIORITY_LOW 1
#define quantum 1000

const int schedulerList[18] = {-1, 0, -1, 0, -1, -1, 0, 1, -1, 1, 0, -1, 0, 1, 0, -1, -1, -1};

struct ucontext_t schedulerContext;
struct ucontext_t mainContext;
struct ucontext_t *activeContext = NULL;
int alarmFlag = 1;

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

    if (activeProcess->pcb->status == RUNNING && !activeProcess->pcb->context.uc_link) {
        
        printf("process terminated");
        dequeue(activeProcess);
        k_process_cleanup(activeProcess -> pcb);
    }

    static int listPointer = 0;

    // if at the end, restart from the first element
    if(listPointer == 18){
        listPointer = 0;
    }

    int num = schedulerList[listPointer];

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

void alarmHandler(int signum) // SIGALRM
{
    printf("inside alarmHandler");
    alarmFlag = 0;
    swapcontext(activeContext, &schedulerContext);
    
}

void setAlarmHandler(void) {
    struct sigaction act;

    act.sa_handler = alarmHandler;
    act.sa_flags = SA_RESTART;
    sigfillset(&act.sa_mask);

    sigaction(SIGALRM, &act, NULL);
}

void setTimer(void) {
    struct itimerval it;

    it.it_interval = (struct timeval){.tv_usec = quantum};
    it.it_value = it.it_interval;

    setitimer(ITIMER_REAL, &it, NULL);
}

void freeStacks(struct pcb *p){
    free(p->context.uc_stack.ss_sp);
}