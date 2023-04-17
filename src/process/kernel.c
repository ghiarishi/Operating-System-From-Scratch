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
    
    Process *newProcess = (Process*) malloc(sizeof(Process));
                                                            
 
    newProcess->pcb = createPcb(*uc, pidCounter, parent->pid, 0, READY);
    
    pidCounter++;
    // printf("Creating new process \n");
    // printf("PID is %d\n", newProcess->pcb->pid);

    return newProcess->pcb;
}

// k_process_kill(Pcb *process, int signal){
    
// }   

void k_process_cleanup(Process *p, int signal) { 

    switch (signal){
    case S_SIGTERM:
        printf("inside sigterm");
        dequeue(p);
        break;

    case S_SIGCONT: 
        printf("inside sigcont");
        break;
    
    default:
        break;
    }

    activeProcess = NULL; //current process becomes null
    activeContext = NULL; //current context becomes null
}

Process *findProcessByPid(int pid){

    Process *temp = highQhead;
    while(temp != NULL){
        if(temp->pcb->pid == pid){
            return temp;
        }
    }

    temp = medQhead;
    while(temp != NULL){
        if(temp->pcb->pid == pid){
            return temp;
        }
    }

    temp = lowQhead;
    while(temp != NULL){
        if(temp->pcb->pid == pid){
            return temp;
        }
    }

    return temp;
}