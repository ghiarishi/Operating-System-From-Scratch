#include "kernel.h"

void setStack(stack_t *stack){
    void *sp = malloc(SIGSTKSZ);
    // Needed to avoid valgrind errors
    VALGRIND_STACK_REGISTER(sp, sp + SIGSTKSZ);

    *stack = (stack_t){.ss_sp = sp, .ss_size = SIGSTKSZ};
}

struct pcb* k_process_create(struct pcb *parent, void (*func)(), char* argv[], int id, int priority) {
    // Create a new process and set its ID and priority level
    
    ucontext_t *uc = (ucontext_t *) malloc(sizeof(ucontext_t));
    getcontext(uc);
    sigemptyset(&uc->uc_sigmask);

    printf("Before SetStack \n");
    setStack(&uc->uc_stack);
    uc->uc_link = &schedulerContext;

    makecontext(uc, func, 3, argv); 
    
    struct Process *newProcess = (struct Process*) malloc(sizeof(struct Process));

    newProcess->pcb = createPcb(*uc, id, id, priority, READY, "echo hello world");
    printf("Creating new process \n");
    printf("PID is %d\n", newProcess->pcb->pid);
    enqueue(newProcess);
    return newProcess->pcb;
}

// k_process_kill(Pcb *process, int signal)

void k_process_cleanup(struct Process *process) {
    changeStatus(activeProcess, activeProcess->pcb->jobID, 0); // Update the process status to terminated
    dequeue(activeProcess); //dequeue from the respective queue
    activeProcess = NULL; //current process becomes null
    activeContext = NULL; //current context becomes null
}