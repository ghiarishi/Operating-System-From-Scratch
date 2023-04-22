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
Process *orphanQhead = NULL; 
Process *orphanQtail = NULL;

Process *blockedQhead = NULL; 
Process *blockedQtail = NULL;

Process *stoppedQhead = NULL; 
Process *stoppedQtail = NULL;

Process *zombieQhead = NULL; 
Process *zombieQtail = NULL;

void terminateProcess(void){

    k_process_cleanup(activeProcess);
    char buf[50];
    buf[0] = '\0';
    char s[10];
    sprintf(s, "[%d]\t", ticks);
    strcat(buf, s);
    strcat(buf,"EXITED\t");
    sprintf(s, "%d\t", activeProcess->pcb->pid);
    strcat(buf,s);
    sprintf(s, "%d\t", activeProcess->pcb->priority);
    strcat(buf,s);
    strcat(buf, activeProcess->pcb->argument);
    strcat(buf,s);
    strcat(buf,"\n");
    writeLogs(buf);

    setcontext(&schedulerContext);
}

void scheduler(void){
    
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

            if (highQhead != NULL) {
                if (highQhead == highQtail) {
                    // If there is only one element in the list, no need to do anything
                } else {
                    // Move the head of the list to the back
                    highQtail->next = highQhead;
                    highQtail = highQhead;
                    highQhead = highQhead->next;
                    highQtail->next = NULL;
                }
            }
            
            emptyQflag = 0;
            listPointer++;
            // printf("setting the context in high Q\n");
            // writeLogs("SCHEDULE %d HIGH %s",activeContext->pcb->pid, activeContext->pcb->argument);
            // char buf[20];
            // buf[0] = '\0';
            // char s[10];
            // strcat(buf,"SCHEDULE\t");
            // sprintf(s, "%d\t", activeProcess->pcb->pid);
            // strcat(buf,s);
            // if (activeProcess->pcb->priority == PRIORITY_HIGH)
            //     sprintf(s, "%s\t", "HIGH");
            // else if (activeProcess->pcb->priority== PRIORITY_LOW)
            //     sprintf(s, "%s\t", "LOW");
            // else if (activeProcess->pcb->priority == PRIORITY_MED)
            //     sprintf(s, "%s\t", "MEDIUM");
            // strcat(buf,s);
            // strcat(buf,activeProcess->pcb->argument);
            // strcat(buf,"\n");
            // writeLogs(buf);

            setcontext(activeContext);
        }
        
        break;     
    case PRIORITY_LOW:
        // printf("In low priority switch\n");
        if (lowQhead != NULL){
            activeProcess = lowQhead;
            activeContext = &lowQhead->pcb->context;
            lowQhead->pcb->status = RUNNING;

            if (lowQhead != NULL) {
                if (lowQhead == lowQtail) {
                    // If there is only one element in the list, no need to do anything
                } else {
                    // Move the head of the list to the back
                    lowQtail->next = lowQhead;
                    lowQtail = lowQhead;
                    lowQhead = lowQhead->next;
                    lowQtail->next = NULL;
                }
            }
            emptyQflag = 0;
            listPointer++;
            // printf("setting the context in low Q\n");
            // writeLogs("SCHEDULE %d LOW %s",activeContext->pcb->pid, activeContext->pcb->argument);
            setcontext(activeContext);
        }
        break;
    
    default:
        // printf("Default of switch case\n");
        if (medQhead != NULL){
            activeProcess = medQhead;
            // printf("%s chosen for execution (med Q Head) \n", activeProcess->pcb->argument);
            // printf("Processes in med Q090909\n");
            // iterateQueue(medQhead);
            activeContext = &medQhead->pcb->context;
            medQhead->pcb->status = RUNNING;


            if (medQhead != NULL) {
                if (medQhead == medQtail) {
                    // If there is only one element in the list, no need to do anything
                } else {
                    // Move the head of the list to the back
                    medQtail->next = medQhead;
                    medQtail = medQhead;
                    medQhead = medQhead->next;
                    medQtail->next = NULL;
                }
            }
            
            // printf("med q head: %s\n", medQhead->pcb->argument);
            // printf("med q tail: %s\n", medQtail->pcb->argument);
            emptyQflag = 0;            
            listPointer++;
            // printf("setting the context in med Q\n");

            // printf("Processes in med Q1111\n");
            // iterateQueue(medQhead);
            // writeLogs("SCHEDULE %d MEDIUM %s",activeContext->pcb->pid, activeContext->pcb->argument);
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
    char buf[50];
    buf[0] = '\0'; 
    char s[10];
    sprintf(s, "[%d]\t", ticks);
    strcat(buf, s);
    strcat(buf,"BLOCKED\t");
    sprintf(s, "%d\t", newProcess->pcb->pid);
    strcat(buf,s);
    sprintf(s, "%d\t", newProcess->pcb->priority);
    strcat(buf,s);
    sprintf(s, "%s", newProcess->pcb->argument);
    strcat(buf,s);
    strcat(buf,"\n");
    writeLogs(buf);

    if (blockedQhead == NULL) {
        blockedQhead = newProcess;
        blockedQtail = newProcess;
    }
    else {
        blockedQtail->next = newProcess;
        blockedQtail = newProcess;
    }
    // printf("Processes in blocked Q\n");
    // iterateQueue(blockedQhead);
    
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
    
    char buf[50];
    buf[0] = '\0'; 
    char s[10];
    sprintf(s, "[%d]\t", ticks);
    strcat(buf, s);
    strcat(buf,"STOPPED\t");
    sprintf(s, "%d\t", newProcess->pcb->pid);
    strcat(buf,s);
    sprintf(s, "%d\t", newProcess->pcb->priority);
    strcat(buf,s);
    sprintf(s, "%s", newProcess->pcb->argument);
    strcat(buf,s);
    strcat(buf,"\n");
    writeLogs(buf);

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
    // printf("Processes in zombie Q\n");
    // iterateQueue(zombieQhead);

    char buf[50];
    buf[0] = '\0'; 
    char s[10];
    sprintf(s, "[%d]\t", ticks);
    strcat(buf, s);
    strcat(buf,"ZOMBIE\t");
    sprintf(s, "%d\t", newProcess->pcb->pid);
    strcat(buf,s);
    sprintf(s, "%d\t", newProcess->pcb->priority);
    strcat(buf,s);
    sprintf(s, "%s", newProcess->pcb->argument);
    strcat(buf,s);
    strcat(buf,"\n");
    writeLogs(buf);
}

void enqueueOrphan(Process* newProcess){
    newProcess->pcb->changedStatus = 1;
    // printf("%s enqueued into orphan Q!\n", newProcess->pcb->argument);
    if (orphanQhead == NULL) {
        orphanQhead = newProcess;
        orphanQtail = newProcess;
    }
    else {
        orphanQtail->next = newProcess;
        orphanQtail = newProcess;
    }
    // printf("Processes in orphan Q\n");
    // iterateQueue(zombieQhead);

    char buf[50];
    buf[0] = '\0'; 
    char s[10];
    sprintf(s, "[%d]\t", ticks);
    strcat(buf, s);
    strcat(buf,"ORPHAN\t");
    sprintf(s, "%d\t", newProcess->pcb->pid);
    strcat(buf,s);
    sprintf(s, "%d\t", newProcess->pcb->priority);
    strcat(buf,s);
    sprintf(s, "%s", newProcess->pcb->argument);
    strcat(buf,s);
    strcat(buf,"\n");
    writeLogs(buf);
}

// Function to add a thread to the appropriate priority queue
void enqueue(Process* newProcess) {
    // Determine the appropriate priority queue based on the ProcessnewProcess's priority level
    newProcess->pcb->status = RUNNING;
    // char buf[20];
    // buf[0] = '\0';
    // char s[20];
    // strcat(buf,"SCHEDULE\t");
    // sprintf(s, "%d\t", newProcess->pcb->pid);
    // strcat(buf,s);
    // if (newProcess->pcb->priority == PRIORITY_HIGH)
    //     sprintf(s, "%s\t", "HIGH");
    // else if (newProcess->pcb->priority== PRIORITY_LOW)
    //     sprintf(s, "%s\t", "LOW");
    // else if (newProcess->pcb->priority == PRIORITY_MED)
    //     sprintf(s, "%s\t", "MEDIUM");
    // strcat(buf,s);
    // if (newProcess->pcb->argument != NULL){
    //     sprintf(s, "%s\t", newProcess->pcb->argument);
    //     strcat(buf,s);
    // }
        
    // strcat(buf,"\n");
    // writeLogs(buf);
    
    switch(newProcess->pcb->priority) {
        case PRIORITY_HIGH:{

            if (highQhead == NULL) {
                highQhead = newProcess;
                highQtail = newProcess;
            }
            else {
                highQtail->next = newProcess;
                highQtail = newProcess;
            }
        }
            // printf("%s enqueued into high\n", newProcess->pcb->argument);

            
            break;
        case PRIORITY_LOW:{
            // char buf[20];
            // buf[0] = '\0'; 
            // char s[10];
            // strcat(buf,"SCHEDULE\t");
            // sprintf(s, "%d\t", newProcess->pcb->pid);
            // strcat(buf,s);
            // strcat(buf,"LOW\t");
            // // strcat(buf,s);
            // strcat(buf,newProcess->pcb->argument);
            // strcat(buf,"\n");
            // writeLogs(buf);
            // printf("%s enqueued into low\n", newProcess->pcb->argument);
            if (lowQhead == NULL) {
                lowQhead = newProcess;
                lowQtail = newProcess;
            }
            else {
                lowQtail->next = newProcess;
                lowQtail = newProcess;
            }
        }
        default:{
            // char buf[20];
            // buf[0] = '\0'; 
            // char s[10];
            // strcat(buf,"SCHEDULE\t");
            // sprintf(s, "%d\t", newProcess->pcb->pid);
            // strcat(buf,s);
            // strcat(buf,"MEDIUM\t");
            // // strcat(buf,s);
            // strcat(buf,newProcess->pcb->argument);
            // strcat(buf,"\n");
            // writeLogs(buf);
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

}

void dequeueZombie(Process* newProcess){
    // if first job, set the new head to the next job and free head
    
    if (zombieQhead != NULL && zombieQhead->pcb->pid == newProcess->pcb->pid) {
        Process *old_head = zombieQhead;
        zombieQhead = zombieQhead->next;
        // printf("%s dequeued from zombie Q head\n", newProcess->pcb->argument);
        old_head->next = NULL;

        // printf("Processes in zombie Q AFTER IMP\n");
        // iterateQueue(zombieQhead);
        return;
    }

    // iterate through all jobs until job of interest is reached
    Process *current = zombieQhead;
    while (current -> next != NULL){

        // if the next job is the one, replace next with the one after that
        if (current -> next -> pcb -> pid == newProcess->pcb->pid){
            // printf("%s deququed from zombie Q\n", newProcess->pcb->argument);
            Process *removed = current -> next;
            Process *newNext = removed -> next;
            if (newNext == NULL){
                zombieQtail = current;
            }
            // if else for stopped or terminated, act differently for both
            current -> next = newNext;
            removed -> next = NULL; 
            // freeOneJob(&removed);
            // printf("Processes in dequeue zombie Q\n");
            // iterateQueue(zombieQhead);
            return;
        }
        current = current -> next;
    }

}

void dequeueOrphan(Process* newProcess){
    // if first job, set the new head to the next job and free head
    
    if (orphanQhead != NULL && orphanQhead->pcb->pid == newProcess->pcb->pid) {
        Process *old_head = orphanQhead;
        orphanQhead = orphanQhead->next;
        // printf("%s dequeued from orphan Q head\n", newProcess->pcb->argument);
        old_head->next = NULL;

        // printf("Processes in orphan Q AFTER IMP\n");
        // iterateQueue(orphanQhead);
        return;
    }

    // iterate through all jobs until job of interest is reached
    Process *current = orphanQhead;
    while (current -> next != NULL){

        // if the next job is the one, replace next with the one after that
        if (current -> next -> pcb -> pid == newProcess->pcb->pid){
            // printf("%s deququed from orphan Q\n", newProcess->pcb->argument);
            Process *removed = current -> next;
            Process *newNext = removed -> next;
            if (newNext == NULL){
                orphanQtail = current;
            }
            // if else for stopped or terminated, act differently for both
            current -> next = newNext;
            removed -> next = NULL; 
            // freeOneJob(&removed);
            // printf("Processes in dequeue orphan Q\n");
            // iterateQueue(orphanQhead);
            return;
        }
        current = current -> next;
    }

}

void dequeueBlocked(Process* newProcess){
    // printf("Processes in blocked Q IMP LOOK AT THIS\n");
    // iterateQueue(blockedQhead);
    char buf[50];
    buf[0] = '\0'; 
    char s[10];
    sprintf(s, "[%d]\t", ticks);
    strcat(buf, s);
    strcat(buf,"UNBLOCKED\t");
    sprintf(s, "%d\t", newProcess->pcb->pid);
    strcat(buf,s);
    sprintf(s, "%d\t", newProcess->pcb->priority);
    strcat(buf,s);
    sprintf(s, "%s", newProcess->pcb->argument);
    strcat(buf,s);
    strcat(buf,"\n");
    writeLogs(buf);

    if (blockedQhead != NULL && blockedQhead->pcb->pid == newProcess->pcb->pid) {
        Process *old_head = blockedQhead;
        blockedQhead = blockedQhead->next;
        // printf("%s dequeued from blocked Q head\n", newProcess->pcb->argument);
        old_head->next = NULL;

        // printf("Processes in blocked Q AFTER IMP\n");
        // iterateQueue(blockedQhead);
        return;
    }

    // iterate through all jobs until job of interest is reached
    Process *current = blockedQhead;
    while (current -> next != NULL){

        // if the next job is the one, replace next with the one after that
        if (current -> next -> pcb -> pid == newProcess->pcb->pid){
            // printf("%s deququed from blocked Q\n", newProcess->pcb->argument);
            Process *removed = current -> next;
            Process *newNext = removed -> next;
            if (newNext == NULL){
                blockedQtail = current;
            }
            current -> next = newNext;
            removed -> next = NULL; 
            // printf("Processes in blocked Q\n");
            // iterateQueue(blockedQhead);
            return;
        }
        current = current -> next;
    }
}

void dequeueStopped(Process* newProcess){
    char buf[50];
    buf[0] = '\0'; 
    char s[10];
    sprintf(s, "[%d]\t", ticks);
    strcat(buf, s);
    strcat(buf,"CONTINUED\t");
    sprintf(s, "%d\t", newProcess->pcb->pid);
    strcat(buf,s);
    sprintf(s, "%d\t", newProcess->pcb->priority);
    strcat(buf,s);
    sprintf(s, "%s", newProcess->pcb->argument);
    strcat(buf,s);
    strcat(buf,"\n");
    writeLogs(buf);
    
    Process *current = NULL;
    // if first job, set the new head to the next job and free head
    if (stoppedQhead->pcb->pid == newProcess->pcb->pid){
        stoppedQhead->next = NULL;
        stoppedQhead = stoppedQhead->next;
        // printf("%s dequeued from stopped (head)\n", newProcess->pcb->argument);
        // freeOneJob(head);
        return;
    }

    // iterate through all jobs until job of interest is reached
    current = stoppedQhead;
    while (current -> next != NULL){
        // if the next job is the one, replace next with the one after that
        if (current -> next -> pcb -> pid == newProcess->pcb->pid){
            // printf("%s dequeued from stopped\n", newProcess->pcb->argument);
            Process *removed = current -> next;
            Process *newNext = removed -> next;
            if (newNext == NULL){
                stoppedQtail = current;
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
                highQhead->next = NULL;
                highQhead = highQhead->next;
                // printf("%s dequeued from high (head)\n", newProcess->pcb->argument);
                // freeOneJob(head);
                return;
            }

            // iterate through all jobs until job of interest is reached
            current = highQhead;
            while (current -> next != NULL){
                // if the next job is the one, replace next with the one after that
                if (current -> next -> pcb -> pid == newProcess->pcb->pid){
                    // printf("%s dequeued from high\n", newProcess->pcb->argument);
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
                lowQhead->next = NULL;
                lowQhead = lowQhead->next;
                // printf("%s dequeued from low (head)\n", newProcess->pcb->argument);
                // freeOneJob(head);
                return;
            }

            // iterate through all jobs until job of interest is reached
            current = lowQhead;
            while (current -> next != NULL){
                // if the next job is the one, replace next with the one after that
                if (current -> next -> pcb -> pid == newProcess->pcb->pid){
                    // printf("%s dequeued from low\n", newProcess->pcb->argument);
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
                medQhead->next = NULL;
                medQhead = medQhead->next;
    
                // printf("med q head : %s\n", medQhead->pcb->argument);
                // printf("%s dequeued from med (head)\n", newProcess->pcb->argument);
                // freeOneJob(head);
                // printf("Processes in med Q\n");
                // iterateQueue(medQhead);
                return;
            }

            // iterate through all jobs until job of interest is reached
            current = medQhead;
            while (current -> next != NULL){
                // if the next job is the one, replace next with the one after that
                if (current -> next -> pcb -> pid == newProcess->pcb->pid){
                    // printf("%s dequeued from med\n", newProcess->pcb->argument);
                    Process *removed = current -> next;
                    Process *newNext = removed -> next;
                    if (newNext == NULL){
                        medQtail = current;
                    }
                    // if else for stopped or terminated, act differently for both
                    current -> next = newNext;
                    removed -> next = NULL; 
                    // freeOneJob(&removed);
                    // printf("Processes in med Q\n");
                    // iterateQueue(medQhead);
                    return;
                }
                current = current -> next;
            }

            // printf("Processes in med Q\n");
            // iterateQueue(medQhead);
    }
}

void iterateQueue(Process *head){
    if(head == NULL){
        printf("nothing in Q\n");
        return;
    }
    while(head!= NULL){
        printf("%s\n", head->pcb->argument);
        head = head->next;
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
void alarmHandler(void){
    ticks++;
    Process *temp = blockedQhead;
    int* array = malloc(20 * sizeof(int));
    int count = 0;
    while(temp != NULL) {

        if(temp->pcb->sleep_time_remaining > 0 && temp->pcb->status != STOPPED){
            temp->pcb->sleep_time_remaining--;
            // printf("%s time remaining = %d\n", temp->pcb->argument, temp->pcb->sleep_time_remaining);
        }
        if(temp->pcb->sleep_time_remaining == 0){
            array[count] = temp->pcb->pid;
            count ++;
            // printf("%s TIME OVER\n", temp->pcb->argument);
        }
        temp = temp->next;
    }
    for(int i = 0; i< count; i++){
        // printf("%d\n", array[i]);
        Process *temp2 = findProcessByPid(array[i]);
        dequeueBlocked(temp2);
        enqueue(temp2);
    }
    // do the dequeuing
    swapcontext(activeContext, &schedulerContext);
}

void freeStacks(struct pcb *p){
    free(p->context.uc_stack.ss_sp);
}