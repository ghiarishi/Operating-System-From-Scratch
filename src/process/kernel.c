#include "kernel.h"

struct Process* createNewProcess(void (*func)(), char* argv[], int id, int priority) {
    // Create a new process and set its ID and priority level
    
    ucontext_t *uc = (ucontext_t *) malloc(sizeof(ucontext_t));
    getcontext(uc);
    sigemptyset(&uc->uc_sigmask);

    printf("Before SetStack \n");
    setStack(&uc->uc_stack);
    uc->uc_link = NULL;

    makecontext(uc, func, 3, argv);
    
    struct Process *newProcess = (struct Process*) malloc(sizeof(struct Process));

    newProcess->pcb = createPcb(*uc, id, id, priority, READY, "echo hello world");
    printf("Creating new process \n");
    printf("PID is %d\n", newProcess->pcb->pid);
    enqueueProcess(newProcess);
    return newProcess;
}