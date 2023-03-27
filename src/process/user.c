#include "user.h"

// static ucontext_t schedulerContext;

void testFunc1() {
    printf("helllo world");
}

pid_t p_spawn(void (*func)(), char *argv[], int fd0, int fd1) {

    printf("inside p_spawn");
    pid_t pid = getpid();
    struct Process *testProcess = createNewProcess(func, argv, pid, -1);
    pid_t pid_new = testProcess->pcb->pid;

    printf("passed by make context");
    return pid_new;
}

// pid_t p_waitpid(pid_t pid, int *wstatus, bool nohang){

// }

// int p_kill(pid_t pid, int sig){

// }

// void p_exit(void){

// }

// k_process_create(Pcb *parent){

// }

// k_process_kill(Pcb *process, int signal){

// }

// k_process_cleanup(Pcb *process){

// }