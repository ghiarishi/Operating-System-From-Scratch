#include "user.h"
#include "kernel.h"
int pidCounter = 1;

Process *activeProcess = NULL;

char* concat(int argc, char *argv[]) {
    char *cmd_line = malloc(MAX_CMD_LENGTH * sizeof(char));
    memset(cmd_line, 0, MAX_CMD_LENGTH);
    
    for (int i = 0; i < argc; i++) {
        strcat(cmd_line, argv[i]);
        strcat(cmd_line, " ");
    }
    return cmd_line;
}

pid_t p_spawn(void (*func)(), char *argv[], int fd0, int fd1) {

    // printf("Inside p_spawn \n");
    pid_t pid_new;

    // pid_t pid = getpid();

    activeProcess->pcb->numChild++;

    Process *newProcess = (Process*) malloc(sizeof(Process)); //NULL;
    newProcess->pcb = k_process_create(activeProcess->pcb);
    pid_new = newProcess->pcb->pid;

    // set up child fd table based on fd0/fd1
    // todo: uncomment this after changing initial shell to use low-level k_process_create to prevent segfault
//    file_t *in_f = activeProcess->pcb->fd_table[fd0];
//    file_t *out_f = activeProcess->pcb->fd_table[fd1];
//    memcpy(newProcess->pcb->fd_table[PSTDIN_FILENO], in_f, sizeof(file_t));
//    memcpy(newProcess->pcb->fd_table[PSTDOUT_FILENO], out_f, sizeof(file_t));
//    fs_trackopen(fs, newProcess->pcb->fd_table[PSTDIN_FILENO]);
//    fs_trackopen(fs, newProcess->pcb->fd_table[PSTDOUT_FILENO]);

    int argc = 0;
    int i = 0;
    while(argv[i] != NULL){
        argc++;
        i ++;
    }

    struct parsed_command *cmd;
    parse_command(concat(argc, argv), &cmd);

    newProcess->pcb->argument = malloc((strlen(concat(argc, argv))+1) * sizeof(char));
    newProcess->pcb->argument = concat(argc, argv);

    // replace with just one call to makecontext
    if(func == (void(*)())&pennShell){
        newProcess->pcb->priority = -1;
        makecontext(&newProcess->pcb->context, func, 0, argv);
    }
    else {
        makecontext(&newProcess->pcb->context, func, 2, argc, cmd->commands[0]);
    }
    printf("newProcess->pid: %d\n", newProcess->pcb->pid);
    enqueue(newProcess);

    return pid_new;
}

pid_t p_waitpid(pid_t pid, int *wstatus, bool nohang) {

    printf("just entered PWAIT\n");
    // Find the child process with the specified pid
    if(pid == -1){ 
        // loop through processes that have changed their status
    }
    Process *p = findProcessByPid(pid);

    // printf("inside waitpid child%d; parent%d\n", p->pcb->pid, activeProcess->pcb->pid);

    activeProcess->pcb->waitChild = pid;
    printf("printing child that has to be waited on: %d\n", activeProcess->pcb->waitChild);
    if(!nohang){
        dequeue(activeProcess);
        enqueueBlocked(activeProcess);
        
        printf("the pid of bqh %d\n", blockedQhead->pcb->pid);
    }
    swapcontext(activeContext, &schedulerContext);
    *wstatus = p->pcb->status;

    int tpid = p->pcb->pid; 
    
    freeStacks(p->pcb);
    freePcb(p->pcb);
    free(p);
    
    // printf("mark 5 \n");

    return tpid;
}

int p_kill(pid_t pid, int sig){

    Process *proc = findProcessByPid(pid);
    // printf("%d\n", sig);
    switch(sig) {
        case S_SIGTERM:
            printf("SIGTERM \n");
            printf("%d\n", proc->pcb->numChild);
            return k_process_kill(proc, S_SIGTERM);

        case S_SIGCONT:
            printf("SIGCONT\n");
            return 0;

        case S_SIGSTOP:
            printf("SIGTERM \n");
            k_process_kill(proc, S_SIGSTOP);
            return 0;
    }
    return -1;
}

void p_sleep(unsigned int ticks){
    dequeue(activeProcess);
    activeProcess->pcb->sleep_time_remaining = ticks;
    enqueueBlocked(activeProcess);
    swapcontext(activeContext, &schedulerContext);
    printf("finished with psleep\n");
}

// void p_exit(void){
//     //should be like killing but don't care about signal. just KILL KILL KILL.
// }