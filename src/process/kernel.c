#include "kernel.h"

void setStack(stack_t *stack){
    void *sp = malloc(SIGSTKSZ);
    // Needed to avoid valgrind errors
    VALGRIND_STACK_REGISTER(sp, sp + SIGSTKSZ);

    *stack = (stack_t){.ss_sp = sp, .ss_size = SIGSTKSZ};
}

struct pcb* k_process_create(struct pcb *parent) {
    // Create a new process and set its ID and priority level
    
    // printf("Inside k_proc_create \n");

    ucontext_t *uc = (ucontext_t*) malloc(sizeof(ucontext_t));
    getcontext(uc);
    sigemptyset(&uc->uc_sigmask);

    setStack(&uc->uc_stack);
    uc->uc_link = &terminateContext;
    
    // Process *newProcess = (Process*) malloc(sizeof(Process));
    struct pcb *p = NULL;
                                                            
    p = createPcb(*uc, pidCounter, parent->pid, 0, RUNNING);
    
    pidCounter++;

    return p;
}

int k_process_kill(Process *p, int signal){

    // printf("inside k  process kill \n");
    switch (signal){
    case S_SIGTERM: 
        if(p->pcb->numChild == 0){
            // printf("SIGTERM 1\n");
            printf("sleep%d \n", p->pcb->status);
            if(p->pcb->status == BLOCKED){
                // sleep case
                // printf("sleep \n");
                Process *parent = findProcessByPid(p->pcb->ppid);
                dequeueBlocked(p);
                dequeueBlocked(parent);
                enqueue(parent);
                // printf("kill BLOCKED\n");
            }
            else if(p->pcb->status == RUNNING){
                Process *parent = findProcessByPid(p->pcb->ppid);
                dequeue(p);
                dequeueBlocked(parent);
                enqueue(parent);
    
                // printf("kill RUNNING \n");
            }
            else if(p->pcb->status == STOPPED){
                dequeueStopped(p);
                // printf("kill STOPPED \n");
            }

            p->pcb->status = SIG_TERMINATED;
            p->pcb->changedStatus = 1;
            return 0;
        }
        for(int i=0; i < p->pcb->numChild; i++){
            // printf("SIGTERM %d\n", i);
            Process *child = findProcessByPid(p->pcb->childPids[i]);
            k_process_kill(child, S_SIGTERM);
        }
        dequeue(p);
        break;
    case S_SIGSTOP:
        dequeue(p);
        enqueueStopped(p);
        break;
    case S_SIGCONT:
        dequeueStopped(p);
        enqueue(p);
        break;
    default:
        break;
    }

    return -1;
}   

void k_process_cleanup(Process *p) { 

    p->pcb->status = TERMINATED;
    p->pcb->changedStatus = 1;
    
    // printf("inside KPC\n");

    dequeue(p);

    Process *temp = blockedQhead;

    while(temp != NULL){
        if(temp->pcb->pid == p->pcb->ppid && (temp->pcb->waitChild == p->pcb->pid)){
            dequeueBlocked(temp);
            for(int i =0;i<temp->pcb->numChild;i++){
                if(temp->pcb->childPids[i] == p->pcb->pid){
                    temp->pcb->childPids[i] = -1;
                }
            }
            enqueue(temp);
            // printf("enq temp\n");
            break;
        }
        temp = temp->next;
    }

    for(int i=0;i<p->pcb->numChild;i++){
        Process *cp = findProcessByPid(p->pcb->childPids[i]);
        k_process_kill(cp, S_SIGTERM);
    }

    // printf("end of KPC\n");
}

Process *findProcessByPid(int pid){

    // printf("entered find process pid\n");
    Process *temp = highQhead;
    while(temp != NULL){
        if(temp->pcb->pid == pid){
            // printf("HIGH exiting find process pid\n");
            return temp;
        }
        temp = temp->next;
    }

    temp = medQhead;
    while(temp != NULL){
        if(temp->pcb->pid == pid){
            // printf("MED exiting find process pid\n");
            return temp;
        }

        temp = temp->next;
    }

    temp = lowQhead;
    while(temp != NULL){
        if(temp->pcb->pid == pid){
            // printf("LOW exiting find process pid\n");
            return temp;
        }
        temp = temp->next;
    }

    temp = blockedQhead;
    while(temp != NULL){
        if(temp->pcb->pid == pid){
            // printf("BLOCKED exiting find process pid\n");
            return temp;
        }
        temp = temp->next;
    }
    
    temp = stoppedQhead;
    while(temp != NULL){
        if(temp->pcb->pid == pid){
            // printf("BLOCKED exiting find process pid\n");
            return temp;
        }
        temp = temp->next;
    }
    printf("Didn't find the process I was looking fo\n");
    return temp;
}