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

    Process *newProcess = (Process*) malloc(sizeof(Process)); //NULL;
    newProcess->pcb = k_process_create(activeProcess->pcb);
    newProcess->next = NULL;
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
        int nums = activeProcess->pcb->numChild;
        activeProcess->pcb->numChild = nums + 1;
        activeProcess->pcb->childPids[nums] = pid_new;
        makecontext(&newProcess->pcb->context, func, 2, argc, cmd->commands[0]);
    }
    // printf("newProcess->pid: %d\n", newProcess->pcb->pid);
    enqueue(newProcess);

    return pid_new;
}

pid_t p_waitpid(pid_t pid, int *wstatus, bool nohang) {

    printf("just entered PWAIT\n");

    int pid_ret = 0;

    if(pid == -1){

        if(activeProcess->pcb->numChild == 0){
            return -1;
        }
        for(int i = 0; i <= activeProcess->pcb->numChild; i++){
            if(activeProcess->pcb->childPids[i] > -2){
                Process *child = findProcessByPid(activeProcess->pcb->childPids[i]);
                if(child == NULL){
                    return 0;
                }
                if(child->pcb->changedStatus == 1){
                    *wstatus = child->pcb->status;
                    pid_ret = activeProcess->pcb->childPids[i];
                    break;
                }
            }
        }
    } else {
        // printf("WAIT ONE JOB \n");
        Process *p = findProcessByPid(pid);
        if(p->pcb->changedStatus == 1){
            activeProcess->pcb->waitChild = pid;
            *wstatus = p->pcb->status;
            pid_ret = pid; 
        }
    }
    if(!nohang){ // FOREGROUND  
        // printf("IN HANG IF \n");  
        // put shell from ready to blocked Q
        activeProcess->pcb->waitChild = pid;
        dequeue(activeProcess);
        enqueueBlocked(activeProcess);
        swapcontext(activeContext, &schedulerContext);
    } 
    else{
        if(zombieQhead != NULL){
            // printf("going into find now \n");
            dequeueZombie(findProcessByPid(pid_ret));
        }
    }

    // printf("CHILD PID = %d\n", pid_ret);
    // printf("About to leave PWAIT\n");
    
    return pid_ret;
}

int p_kill(pid_t pid, int sig){
    // printf("p_kill, pid is %d\n",pid);
    Process *proc = findProcessByPid(pid);
    if(proc == NULL){
        printf("issue here\n");
    }
    printf("pid of proc is %d\n", proc->pcb->pid);
    // printf("%d\n", sig);
    switch(sig) {
        case S_SIGTERM:
            // printf("SIGTERM \n");
            // printf("%d\n", proc->pcb->numChild);
            return k_process_kill(proc, S_SIGTERM);

        case S_SIGCONT:
            printf("SIGCONT\n");
            return k_process_kill(proc, S_SIGCONT);

        case S_SIGSTOP:
            printf("SIGSTOP \n");
            return k_process_kill(proc, S_SIGSTOP);;
    }
    return -1;
}

void p_sleep(unsigned int ticks){
    dequeue(activeProcess);
    activeProcess->pcb->sleep_time_remaining = ticks;
    enqueueBlocked(activeProcess);
    swapcontext(activeContext, &schedulerContext);
    // printf("finished with psleep\n");
}

void p_exit(void){
    // printf("p_exit\n");
    //do cleanup to avoid memory leaks
    return;
}

int p_nice(pid_t pid, int priority){
    Process *proc = findProcessByPid(pid);
    if (proc == NULL){
        return -1;
    }
    dequeue(proc);
    proc->pcb->priority = priority;
    enqueue(proc);
    return 0;
}

