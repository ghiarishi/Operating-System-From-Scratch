#include "scheduler.h"

#define PRIORITY_HIGH -1
#define PRIORITY_MED 0
#define PRIORITY_LOW 1

static const int quantum = 100000;

const int schedulerList[18] = {-1, 0, -1, 0 -1, -1, 0, 1, -1, 1, 0, -1, 0, 1, 0, -1, -1, -1};

struct ucontext_t schedulerContext;
struct ucontext_t terminateContext;
struct ucontext_t *activeContext = NULL;
int emptyQflag = 1;

Process *highQhead = NULL; //extern
Process *highQtail = NULL;
Process *medQhead = NULL;
Process *medQtail = NULL;
Process *lowQhead = NULL; 
Process *lowQtail = NULL;

Process *blockedQhead = NULL; 
Process *blockedQtail = NULL;

void terminateProcess(void){
    
    printf("TIME TO DEQUEUE\n");
    activeProcess->pcb->status = TERMINATED;
    dequeue(activeProcess);
    setcontext(&schedulerContext);

    // raise(S_SIGTERM);
}

void scheduler(void){
    
    // printf("Inside scheduler\n");

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

            if(highQhead != highQtail){
                highQtail->next = activeProcess;
                highQtail = highQtail->next;
                highQtail->next = NULL;
                highQhead = highQhead->next;
            }
            
            emptyQflag = 0;
            listPointer++;
            // printf("setting the context in high Q\n");
            setcontext(activeContext);
        }
        
        break;     
    case PRIORITY_LOW:
        if (lowQhead != NULL){
            activeProcess = lowQhead;
            activeContext = &lowQhead->pcb->context;
            lowQhead->pcb->status = RUNNING;

            if(lowQhead != lowQtail){
                lowQtail->next = activeProcess;
                lowQtail = lowQtail->next;
                lowQtail->next = NULL;
                lowQhead = lowQhead->next;
            }


            emptyQflag = 0;
            listPointer++;
            setcontext(activeContext);
        }
        break;
    
    default:
        if (medQhead != NULL){
            activeProcess = medQhead;
            activeContext = &medQhead->pcb->context;
            medQhead->pcb->status = RUNNING;

            if(medQhead != medQtail){
                medQtail->next = activeProcess;
                medQtail = medQtail->next;
                medQtail->next = NULL;
                medQhead = medQhead->next;
            }

            emptyQflag = 0;            
            listPointer++;
            setcontext(activeContext);
        } 
        else {
            // printf("empty med Q\n");
        }
        break;
    }

    if(highQhead == NULL && medQhead == NULL && lowQhead == NULL){
        emptyQflag = 1;
        printf("All heads nulls\n");
        // run idle process here
    }
    if(!emptyQflag){
        listPointer++;
        setcontext(&schedulerContext);
    }

}

void initContext(void){
    
    getcontext(&schedulerContext);
    schedulerContext.uc_stack.ss_sp = malloc(SIGSTKSZ);
    schedulerContext.uc_stack.ss_size = SIGSTKSZ;
    schedulerContext.uc_link = NULL;
    makecontext(&schedulerContext, scheduler, 0);

    getcontext(&terminateContext);
    terminateContext.uc_stack.ss_sp = malloc(SIGSTKSZ);
    terminateContext.uc_stack.ss_size = SIGSTKSZ;
    terminateContext.uc_link = NULL;
    makecontext(&terminateContext, terminateProcess, 0);
}

// SIGALRM
void alarmHandler(int signum){
    if(!emptyQflag){
        // printf("SWAPPING CONTEXT \n");
        swapcontext(activeContext, &schedulerContext);
    }
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