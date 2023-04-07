#include "user.h"

int pidCounter = 1;
struct Process *activeProcess = NULL;

pid_t p_spawn(void (*func)(), char *argv[], int fd0, int fd1) {

    printf("Inside p_spawn \n");
    pid_t pid_new;
    
    pid_t pid = getpid();

    struct Process *newProcess = NULL;
    newProcess->pcb = k_process_create(activeProcess->pcb);
    pid_new = newProcess->pcb->pid;

    struct parsed_command *cmd;
    parse_command(argv, &cmd);

    if(func == "penn_shell"){
        makecontext(&newProcess->pcb->context, func, 0, argv);
    } else {
        makecontext(&newProcess->pcb->context, func, cmd->num_commands, cmd->commands[0]);
    }
    enqueue(newProcess);

    return pid_new;
}

// pid_t p_waitpid(pid_t pid, int *wstatus, bool nohang){

// }

// int p_kill(pid_t pid, int sig){

// }

// void p_exit(void){

// }