#include "kernel.h"

void setStack(stack_t *stack){
    void *sp = malloc(SIGSTKSZ);
    // Needed to avoid valgrind errors
    VALGRIND_STACK_REGISTER(sp, sp + SIGSTKSZ);

    *stack = (stack_t){.ss_sp = sp, .ss_size = SIGSTKSZ};
}

struct pcb* k_process_create(struct pcb *parent) {
    // Create a new process and set its ID and priority level
    
    printf("Inside k_proc_create \n");

    ucontext_t *uc = (ucontext_t*) malloc(sizeof(ucontext_t));
    getcontext(uc);
    sigemptyset(&uc->uc_sigmask);

    setStack(&uc->uc_stack);
    uc->uc_link = &terminateContext;
    
    Process *newProcess = (Process*) malloc(sizeof(Process));
                                                            
 
    newProcess->pcb = createPcb(*uc, pidCounter, parent->pid, 0, RUNNING);
    
    pidCounter++;

    return newProcess->pcb;
}

void k_process_kill(Process *p, int signal){

    p->pcb->status = TERMINATED;

    switch (signal){
    case S_SIGTERM: {
    // dequeue p from ready q to zombie q
    }
        // for(int i=0;i<p->pcb->numChild;i++){
        //     Process *cp = findProcessByPid(p->pcb->childPids[i]);
        //     k_process_kill(cp, S_SIGTERM);
        //     dequeue(cp);
        // }
        break;
    case S_SIGSTOP:{
    // dequeue p from stopped q to ready q
    }
    break;
    case S_SIGCONT:{

    }
    break;
    default:
        break;
    }
}   

void k_process_cleanup(Process *p) { 

    p->pcb->status = TERMINATED;

    printf("inside KPC\n");

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

    // printf("before for loop\n");

    for(int i=0;i<p->pcb->numChild;i++){
        Process *cp = findProcessByPid(p->pcb->childPids[i]);
        k_process_kill(cp, S_SIGTERM);
    }

    // printf("head of high Q is : %d\n", highQhead->pcb->pid);
    printf("end of KPC\n");
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

    
    printf("NONE exiting find process pid\n");
    return temp;
}