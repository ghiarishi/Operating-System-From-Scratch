#include "kernel.h"

void setStack(stack_t *stack){
    void *sp = malloc(SIGSTKSZ);
    // Needed to avoid valgrind errors
    VALGRIND_STACK_REGISTER(sp, sp + SIGSTKSZ);

    *stack = (stack_t){.ss_sp = sp, .ss_size = SIGSTKSZ};
}

struct pcb* k_process_create(struct pcb *parent, void (*func)(), char* argv[], int pid, int priority) {
    // Create a new process and set its ID and priority level
    
    ucontext_t *uc = (ucontext_t *) malloc(sizeof(ucontext_t));
    getcontext(uc);
    sigemptyset(&uc->uc_sigmask);

    printf("Before SetStack \n");
    setStack(&uc->uc_stack);
    uc->uc_link = &schedulerContext;

    makecontext(uc, func, strlen(argv), argv); 
    
    struct Process *newProcess = (struct Process*) malloc(sizeof(struct Process));
                                                                        
    pidCounter++; 
    newProcess->pcb = createPcb(*uc, pidCounter, parent->pid, priority, READY, NULL);
    printf("Creating new process \n");
    printf("PID is %d\n", newProcess->pcb->pid);
    enqueue(newProcess);

    return newProcess->pcb;
}

// k_process_kill(Pcb *process, int signal)

void k_process_cleanup(struct Process *p) {
    changeStatus(p, p->pcb->jobID, 0); // Update the process status to terminated
    dequeue(p); //dequeue from the respective queue
    activeProcess = NULL; //current process becomes null
    activeContext = NULL; //current context becomes null
}