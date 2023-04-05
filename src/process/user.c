#include "user.h"

struct Process *activeProcess = NULL;

pid_t p_spawn(void (*func)(), char *argv[], int fd0, int fd1) {

    printf("Inside p_spawn \n");
    pid_t pid_new;
    
    pid_t pid = getpid();

    struct Process *newProcess = NULL;
    newProcess->pcb = k_process_create(activeProcess->pcb, func, argv, pid, 0);
    pid_new = newProcess->pcb->pid;
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