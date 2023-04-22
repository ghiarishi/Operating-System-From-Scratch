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

void removeNicePrefix(char *argv[], int argc) {
     // Ensure there are at least two arguments
    if (argc >= 3) {
        // Move the remaining arguments two positions to the left
        for (int i = 0; i < argc - 2; i++) {
            argv[i] = argv[i + 2];
        }

        // Null-terminate the argv array at the correct position
        argv[argc - 2] = NULL;
        argv[argc - 1] = NULL;
    } else {
        fprintf(stderr, "Not enough arguments to remove first two tokens.\n");
    }
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
    // file_t *in_f = activeProcess->pcb->fd_table[fd0];
    // file_t *out_f = activeProcess->pcb->fd_table[fd1];
    // memcpy(newProcess->pcb->fd_table[PSTDIN_FILENO], in_f, sizeof(file_t));
    // memcpy(newProcess->pcb->fd_table[PSTDOUT_FILENO], out_f, sizeof(file_t));
    // fs_trackopen(fs, newProcess->pcb->fd_table[PSTDIN_FILENO]);
    // fs_trackopen(fs, newProcess->pcb->fd_table[PSTDOUT_FILENO]);

    int argc = 0;
    int i = 0;
    while(argv[i] != NULL){
        argc++;
        i ++;
    }
    if(strcmp(argv[0], "nice") == 0){
        removeNicePrefix(argv, 4);
        argc = 2;
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
    enqueue(newProcess);

    return pid_new;
}



pid_t p_waitpid(pid_t pid, int *wstatus, bool nohang) {
    int pidRet = 0;
    if(nohang){
        if(pid == -1){
            // printf("mark 1\n");
            if(zombieQhead != NULL){
                // printf("mark 2\n");
                pidRet = zombieQhead->pcb->pid;
                zombieQhead->pcb->changedStatus = 0;
                *wstatus = zombieQhead->pcb->status;
                // printf("mark 3\n");
                // printf("ARG ARG ARG %s\n", zombieQhead->pcb->argument);
                dequeueZombie(zombieQhead);
                // printf("mark 4\n");
                for(int i = 0; i < activeProcess->pcb->numChild; i++){
                    // printf("child pids are: %d\n", activeProcess->pcb->childPids[i]);
                    if(activeProcess -> pcb -> childPids[i] == pidRet){
                        activeProcess ->pcb->childPids[i] = -2;
                        break;
                    }
                }
                // printf("mark 5\n");
                return pidRet;
            } 
            return -1;
        } else {
            Process *curr = zombieQhead;
            while(curr != NULL){
               if(curr->pcb->pid == pid){
                    pidRet = curr->pcb->pid;
                    curr->pcb->changedStatus = 0;
                    *wstatus = zombieQhead->pcb->status;
                    dequeueZombie(curr);
                    for(int i = 0; i < activeProcess->pcb->numChild; i++){
                        if(activeProcess -> pcb -> childPids[i] == pid){
                            activeProcess ->pcb->childPids[i] = -2;
                            break;
                        }
                    }
                    return pidRet;
               } 
               curr = curr->next;
            }
            return -1;
        }
    }
    else{// hang
        if(pid != -1){
            Process *fgproc = findProcessByPid(pid);
            // printf("args is %s\n", fgproc->pcb->argument);
            if(fgproc->pcb->changedStatus == 1){ // if there is change of status already
                pidRet = fgproc->pcb->pid;
                *wstatus = fgproc->pcb->status;
                fgproc->pcb->changedStatus = 0;
                for(int i = 0; i < activeProcess->pcb->numChild; i++){
                    if(activeProcess -> pcb -> childPids[i] == pid && fgproc->pcb->status != STOPPED){
                        activeProcess ->pcb->childPids[i] = -2;
                        break;
                    }
                }
                return pidRet;
            } else{ // if there isnt currently a change of status YET
                activeProcess->pcb->waitChild = pid;
                dequeue(activeProcess);
                enqueueBlocked(activeProcess);
                swapcontext(activeContext, &schedulerContext);
                // printf("WAIT HO GAYA BUSY KA BC\n");
                // printf("%s has status: %d", fgproc->pcb->argument, fgproc->pcb->changedStatus);
                if(fgproc->pcb->changedStatus == 1){
                    *wstatus = fgproc->pcb->status;
                    fgproc->pcb->changedStatus = 0;
                    for(int i = 0; i < activeProcess->pcb->numChild; i++){
                        if(activeProcess -> pcb -> childPids[i] == pid && fgproc->pcb->status != STOPPED){
                            activeProcess ->pcb->childPids[i] = -2;
                            break;
                        }
                    }
                    return pidRet;
                }
                return -1;
            }
        } else {
            for(int i = 0;i<activeProcess->pcb->numChild;i++){
                Process *cproc = findProcessByPid(activeProcess->pcb->childPids[i]);
                if(activeProcess ->pcb->childPids[i] != -2 && cproc->pcb->changedStatus == 1){
                    *wstatus = cproc->pcb->status;
                    pidRet = cproc->pcb->pid;
                    cproc->pcb->changedStatus = 0;
                    if(cproc->pcb->status != STOPPED){
                        activeProcess ->pcb->childPids[i] = -2;
                        return pidRet;
                    }
                    return pidRet;
                }
            }
            return -1;
        }
    }
}

int p_kill(pid_t pid, int sig){
    // printf("p_kill, pid is %d\n",pid);
    Process *proc = findProcessByPid(pid);
    if(proc == NULL){
        printf("issue here\n");
    }
    // printf("pid of proc is %d\n", proc->pcb->pid);
    // printf("%d\n", sig);
    switch(sig) {
        case S_SIGTERM:
            // printf("SIGTERM \n");
            return k_process_kill(proc, S_SIGTERM);

        case S_SIGCONT:
            // printf("SIGCONT\n");
            return k_process_kill(proc, S_SIGCONT);

        case S_SIGSTOP:
            // printf("SIGSTOP \n");
            return k_process_kill(proc, S_SIGSTOP);
    }
    return -1;
}

void p_sleep(unsigned int ticks){
    // printf("Just entered %s\n", activeProcess->pcb->argument);
    dequeue(activeProcess);
    // printf("MARK 1\n");
    activeProcess->pcb->sleep_time_remaining = ticks;
    // printf("MARK 2\n");
    enqueueBlocked(activeProcess);
    // printf("MARK 3\n");
    swapcontext(activeContext, &schedulerContext);
    if(activeProcess->pcb->sleep_time_remaining > 0){
        dequeue(activeProcess);
        enqueueBlocked(activeProcess);
    }
}

void p_exit(void){
    // printf("p_exit\n");
    //do cleanup to avoid memory leaks
    // fclose(fp);
    return;
}

int p_nice(pid_t pid, int priority){
    
    Process *proc = findProcessByPid(pid);
    if (proc == NULL){
        return -1;
    }
    switch(proc->pcb->status){
        case RUNNING:
            dequeue(proc);
            proc->pcb->priority = priority;
            enqueue(proc);
            break;

        case BLOCKED:
            proc->pcb->priority = priority;
            break;

        case STOPPED:
            proc->pcb->priority = priority;
            break;

        default:
            return -1;
    }
       
    return 1;
}

