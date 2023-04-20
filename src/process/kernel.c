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
    case S_SIGTERM: {
        // printf("SIGTERM k_kill\n");
        if(p->pcb->numChild == 0){
            // printf("SIGTERM 1\n");
            // printf("kproc kill 0 child %d \n", p->pcb->status);
            if(p->pcb->status == BLOCKED){
                Process *parent = findProcessByPid(p->pcb->ppid);
                dequeueBlocked(p);
                enqueueZombie(p);
                dequeueBlocked(parent);
                enqueue(parent);
                // printf("kill BLOCKED\n");
            }
            else if(p->pcb->status == RUNNING){
                Process *parent = findProcessByPid(p->pcb->ppid);
                dequeue(p);
                enqueueZombie(p);
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
    }
    break;
    case S_SIGSTOP:{
        Process *parent = findProcessByPid(p->pcb->ppid);

        if(p->pcb->status == RUNNING){
            dequeue(p);
        }
        else if(p->pcb->status == BLOCKED){
            dequeueBlocked(p);
        }
        enqueueStopped(p);
        if (parent->pcb->status == BLOCKED){
            dequeueBlocked(parent);
            enqueue(parent);
        }
        p->pcb->status = STOPPED;
        p->pcb->changedStatus = 1;
        return 0;
    }
    break;
    case S_SIGCONT:
        if (p->pcb->status == STOPPED){
            dequeueStopped(p);
            p->pcb->status = RUNNING;
            enqueue(p);
        }
        break;
    default:
        return -1;
    }
    return -1;
}   

void k_process_cleanup(Process *p) { 

    p->pcb->status = TERMINATED;
    p->pcb->changedStatus = 1;
    
    dequeue(p);

    // printf("inside KPC\n");
    // printf("bg flag = %d\n", p->pcb->bgFlag);
    
    if(p->pcb->bgFlag == 1){
        // is background, so make zombie
        p->pcb->changedStatus = 1;
        enqueueZombie(p);
        return;
    }
    Process *temp = blockedQhead;

    // printf("%d\n", temp->pcb->pid);
    
    while(temp != NULL){
        // printf("ppid is: %d pid is: %d waitchild: %d\n", p->pcb->ppid, temp->pcb->pid, temp->pcb->waitChild);
        if(temp->pcb->pid == p->pcb->ppid && (temp->pcb->waitChild == p->pcb->pid)){
            dequeueBlocked(temp);
            enqueue(temp);
            // printf("enq temp\n");
            break;
        }
        temp = temp->next;
    }

    if(p->pcb->numChild != 0){
        for(int i=0;i<p->pcb->numChild;i++){
            Process *cp = findProcessByPid(p->pcb->childPids[i]);
            k_process_kill(cp, S_SIGTERM);
        }
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
        // printf("pid of med q proc %d %d\n", temp->pcb->pid, pid);
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
    temp = zombieQhead;
    while(temp != NULL){
        if(temp->pcb->pid == pid){
            // printf("BLOCKED exiting find process pid with pid = %d\n", temp->pcb->pid);
            return temp;
        }
        temp = temp->next;
    }
    printf("Didn't find the process I was looking for.\n");
    return NULL;
}