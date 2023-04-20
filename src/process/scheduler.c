#include "scheduler.h"

#define PRIORITY_HIGH -1
#define PRIORITY_MED 0
#define PRIORITY_LOW 1

const int schedulerList[18] = {-1, 0, -1, 0 -1, -1, 0, 1, -1, 1, 0, -1, 0, 1, 0, -1, -1, -1};

struct ucontext_t schedulerContext;
struct ucontext_t terminateContext;
struct ucontext_t idleContext;
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

Process *stoppedQhead = NULL; 
Process *stoppedQtail = NULL;

Process *zombieQhead = NULL; 
Process *zombieQtail = NULL;

void terminateProcess(void){
    printf("Process terminated, about to be dequeued\n");
    k_process_cleanup(activeProcess);
    printf("cleanup done fully \n");
    setcontext(&schedulerContext);
}

void scheduler(void){
    
    // printf("Inside scheduler\n");
    // Process *tempPtr = medQhead;
    // printf("med Q is: ");
    // while (tempPtr != NULL) {
    //     printf("%d ", tempPtr->pcb->pid);
    //     tempPtr = tempPtr->next;
    // }
    // printf("\n");
    // findProcessByPid(26);


    static int listPointer = 0;

    // if at the end, restart from the first element
    if(listPointer == 18){
        listPointer = 0;
    }

    int num = schedulerList[listPointer];

    switch (num){
    case PRIORITY_HIGH:
        // printf("In high priority switch\n");
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
        // printf("In low priority switch\n");
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
            // printf("setting the context in low Q\n");
            setcontext(activeContext);
        }
        break;
    
    default:
        // printf("Default of switch case\n");
        if (medQhead != NULL){
            activeProcess = medQhead;
            // printf("%s taken from med Q \n", activeProcess->pcb->argument);
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
            // printf("setting the context in med Q\n");
            setcontext(activeContext);
        } 
    }

    if(highQhead == NULL && medQhead == NULL && lowQhead == NULL && blockedQhead != NULL){
        emptyQflag = 1;  
        // printf("All queues empty, running idle proc now!\n");
        activeContext = &idleContext;
        setcontext(activeContext);
    }

    if(highQhead == NULL && medQhead == NULL && lowQhead == NULL && blockedQhead == NULL){
        return;
    }
    listPointer++;
    setcontext(&schedulerContext);
}

void enqueueBlocked(Process* newProcess){
    newProcess->pcb->status = BLOCKED;
    newProcess->pcb->changedStatus = 1;
    // printf("%s enqueued into blocked Q!\n", newProcess->pcb->argument);
    if (blockedQhead == NULL) {
        blockedQhead = newProcess;
        blockedQtail = newProcess;
    }
    else {
        blockedQtail->next = newProcess;
        blockedQtail = newProcess;
    }
}

void enqueueStopped(Process* newProcess){
    // printf("%s enqueued into stopped Q!\n", newProcess->pcb->argument);
    if (stoppedQhead == NULL) {
        stoppedQhead = newProcess;
        stoppedQtail = newProcess;
    }
    else {
        stoppedQtail->next = newProcess;
        stoppedQtail = newProcess;
    }
    newProcess->pcb->status = STOPPED;
    newProcess->pcb->changedStatus = 1;
}   

void enqueueZombie(Process* newProcess){
    newProcess->pcb->changedStatus = 1;
    // printf("%s enqueued into zombie Q!\n", newProcess->pcb->argument);
    if (zombieQhead == NULL) {
        zombieQhead = newProcess;
        zombieQtail = newProcess;
    }
    else {
        zombieQtail->next = newProcess;
        zombieQtail = newProcess;
    }
}

// Function to add a thread to the appropriate priority queue
void enqueue(Process* newProcess) {
    // Determine the appropriate priority queue based on the ProcessnewProcess's priority level
    newProcess->pcb->status = RUNNING;
    switch(newProcess->pcb->priority) {
        case PRIORITY_HIGH:
            // printf("%s enqueued into high\n", newProcess->pcb->argument);
            if (highQhead == NULL) {
                highQhead = newProcess;
                highQtail = newProcess;
            }
            else {
                highQtail->next = newProcess;
                highQtail = newProcess;
            }
            break;
        case PRIORITY_LOW:
            // printf("%s enqueued into low\n", newProcess->pcb->argument);
            if (lowQhead == NULL) {
                lowQhead = newProcess;
                lowQtail = newProcess;
            }
            else {
                lowQtail->next = newProcess;
                lowQtail = newProcess;
            }
            break;
        default:
            printf("%s enqueued into med\n", newProcess->pcb->argument);
            if (medQhead == NULL) {
                medQhead = newProcess;
                medQtail = newProcess;
            }
            else {
                medQtail->next = newProcess;
                medQtail = newProcess;
            }
    }
}

void dequeueZombie(Process* newProcess){
    // if first job, set the new head to the next job and free head
    if (zombieQhead->pcb->pid == newProcess->pcb->pid){
        zombieQhead = zombieQhead->next;
        printf("%s deququed from zombie Q\n", newProcess->pcb->argument);
        // freeOneJob(head);
        return;
    }

    // iterate through all jobs until job of interest is reached
    Process *current = zombieQhead;
    while (current -> next != NULL){

        // if the next job is the one, replace next with the one after that
        if (current -> next -> pcb -> pid == newProcess->pcb->pid){
            printf("%s deququed from zombie Q\n", newProcess->pcb->argument);
            Process *removed = current -> next;
            Process *newNext = removed -> next;
            if (newNext == NULL){
                zombieQtail = current;
            }
            // if else for stopped or terminated, act differently for both
            current -> next = newNext;
            removed -> next = NULL; 
            // freeOneJob(&removed);
            return;
        }
        current = current -> next;
    }
}

void dequeueBlocked(Process* newProcess){
    // if first job, set the new head to the next job and free head
    if (blockedQhead->pcb->pid == newProcess->pcb->pid){
        blockedQhead = blockedQhead->next;
        printf("%s deququed from blocked Q\n", newProcess->pcb->argument);
        // freeOneJob(head);
        return;
    }

    // iterate through all jobs until job of interest is reached
    Process *current = blockedQhead;
    while (current -> next != NULL){

        // if the next job is the one, replace next with the one after that
        if (current -> next -> pcb -> pid == newProcess->pcb->pid){
            printf("%s deququed from blocked Q\n", newProcess->pcb->argument);
            Process *removed = current -> next;
            Process *newNext = removed -> next;
            if (newNext == NULL){
                blockedQtail = current;
            }
            // if else for stopped or terminated, act differently for both
            current -> next = newNext;
            removed -> next = NULL; 
            // freeOneJob(&removed);
            return;
        }
        current = current -> next;
    }
}

void dequeueStopped(Process* newProcess){

    // if first job, set the new head to the next job and free head
    if (stoppedQhead->pcb->pid == newProcess->pcb->pid){
        stoppedQhead = stoppedQhead->next;
        printf("%s deququed from stopped Q\n", newProcess->pcb->argument);
        // freeOneJob(head);
        return;
    }

    // iterate through all jobs until job of interest is reached
    Process *current = stoppedQhead;
    while (current -> next != NULL){

        // if the next job is the one, replace next with the one after that
        if (current -> next -> pcb -> pid == newProcess->pcb->pid){
            printf("%s deququed from stopped Q\n", newProcess->pcb->argument);
            Process *removed = current -> next;
            Process *newNext = removed -> next;
            if (newNext == NULL){
                stoppedQhead = current;
            }
            // if else for stopped or terminated, act differently for both
            current -> next = newNext;
            removed -> next = NULL; 
            // freeOneJob(&removed);
            return;
        }
        current = current -> next;
    }
}


void dequeue(Process* newProcess){

    Process *current = NULL;
    switch(newProcess->pcb->priority) {
        case PRIORITY_HIGH:
            // if first job, set the new head to the next job and free head
            if (highQhead->pcb->pid == newProcess->pcb->pid){
                highQhead = highQhead->next;
                printf("%s dequeued from high (head)\n", newProcess->pcb->argument);
                // freeOneJob(head);
                return;
            }

            // iterate through all jobs until job of interest is reached
            current = highQhead;
            while (current -> next != NULL){
                // if the next job is the one, replace next with the one after that
                if (current -> next -> pcb -> pid == newProcess->pcb->pid){
                    printf("%s dequeued from high\n", newProcess->pcb->argument);
                    Process *removed = current -> next;
                    Process *newNext = removed -> next;
                    if (newNext == NULL){
                        highQtail = current;
                    }
                    // if else for stopped or terminated, act differently for both
                    current -> next = newNext;
                    removed -> next = NULL; 
                    // freeOneJob(&removed);
                    return;
                }
                current = current -> next;
            }
            break;
        case PRIORITY_LOW:
            // if first job, set the new head to the next job and free head
            if (lowQhead->pcb->pid == newProcess->pcb->pid){
                lowQhead = lowQhead->next;
                printf("%s dequeued from low (head)\n", newProcess->pcb->argument);
                // freeOneJob(head);
                return;
            }

            // iterate through all jobs until job of interest is reached
            current = lowQhead;
            while (current -> next != NULL){
                // if the next job is the one, replace next with the one after that
                if (current -> next -> pcb -> pid == newProcess->pcb->pid){
                    printf("%s dequeued from low\n", newProcess->pcb->argument);
                    Process *removed = current -> next;
                    Process *newNext = removed -> next;
                    if (newNext == NULL){
                        lowQtail = current;
                    }
                    // if else for stopped or terminated, act differently for both
                    current -> next = newNext;
                    removed -> next = NULL; 
                    // freeOneJob(&removed);
                    return;
                }
                current = current -> next;
            }
            break;
        default:
            // if first job, set the new head to the next job and free head
            if (medQhead->pcb->pid == newProcess->pcb->pid){
                medQhead = medQhead->next;
                printf("%s dequeued from med (head)\n", newProcess->pcb->argument);
                // freeOneJob(head);
                return;
            }

            // iterate through all jobs until job of interest is reached
            current = medQhead;
            while (current -> next != NULL){
                // if the next job is the one, replace next with the one after that
                if (current -> next -> pcb -> pid == newProcess->pcb->pid){
                    printf("%s dequeued from med\n", newProcess->pcb->argument);
                    Process *removed = current -> next;
                    Process *newNext = removed -> next;
                    if (newNext == NULL){
                        medQtail = current;
                    }
                    // if else for stopped or terminated, act differently for both
                    current -> next = newNext;
                    removed -> next = NULL; 
                    // freeOneJob(&removed);
                    return;
                }
                current = current -> next;
            }
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

    getcontext(&idleContext);
    idleContext.uc_stack.ss_sp = malloc(SIGSTKSZ);
    idleContext.uc_stack.ss_size = SIGSTKSZ;
    idleContext.uc_link = NULL;
    makecontext(&idleContext, idleFunc, 0);
}

// SIGALRM
void alarmHandler(int signum){
    Process *temp = blockedQhead;
    while(temp != NULL) {
        if(temp->pcb->sleep_time_remaining > 0){
            temp->pcb->sleep_time_remaining--;
            if(temp->pcb->sleep_time_remaining == 0){
                dequeueBlocked(temp);
                enqueue(temp);
            }
        }
        temp = temp->next;
    }
    swapcontext(activeContext, &schedulerContext);
}

void freeStacks(struct pcb *p){
    free(p->context.uc_stack.ss_sp);
}