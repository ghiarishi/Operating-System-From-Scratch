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

// pid_t p_waitpid(pid_t pid, int *wstatus, bool nohang) {
//     // check if pid is given
//     if(pid != -1){ // look at only one job, specified by pid
//         // only one process 
//         Process *child = findProcessByPid(pid);
//         if(child->pcb->changedStatus == 1){ // look if there is a change of status in the past
//             *wstatus = child->pcb->status;
//             for(int i = 0; i < activeProcess->pcb->numChild; i++){
//                 // iterate through every child 
//                 if(activeProcess->pcb->childPids[i] == pid){
//                     // set to -2 (DONT WAIT ON THIS AGAIN)
//                     activeProcess->pcb->childPids[i] = -2;
//                     break;
//                 }
//             }
//             return child->pcb->pid;
//         } else {
//             if(!nohang){
//                 // enq parent to block
//                 activeProcess->pcb->waitChild = pid;
//                 dequeue(activeProcess);
//                 enqueueBlocked(activeProcess);
//             } else {

//             }
//         }
//     }
//     // 
  
//     return pid_ret;
// }

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
                dequeueZombie(zombieQhead);
                // printf("mark 3\n");
                for(int i = 0; i < activeProcess->pcb->numChild; i++){
                    if(activeProcess -> pcb -> childPids[i] == pidRet){
                        activeProcess ->pcb->childPids[i] = -2;
                        break;
                    }
                }
                // printf("mark 4\n");
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
        Process *fgproc = findProcessByPid(pid);
        if(fgproc->pcb->changedStatus == 1){
            pidRet = fgproc->pcb->pid;
            *wstatus = fgproc->pcb->status;
            fgproc->pcb->changedStatus = 0;
            for(int i = 0; i < activeProcess->pcb->numChild; i++){
                if(activeProcess -> pcb -> childPids[i] == pid){
                    activeProcess ->pcb->childPids[i] = -2;
                    break;
                }
            }
            return pidRet;
        }
        else{
            activeProcess->pcb->waitChild = pid;
            dequeue(activeProcess);
            enqueueBlocked(activeProcess);
            swapcontext(activeContext, &schedulerContext);
            if(fgproc->pcb->changedStatus == 1){
                *wstatus = fgproc->pcb->status;
                fgproc->pcb->changedStatus = 0;
                for(int i = 0; i < activeProcess->pcb->numChild; i++){
                    if(activeProcess -> pcb -> childPids[i] == pid){
                        activeProcess ->pcb->childPids[i] = -2;
                        break;
                    }
                }
                return pidRet;
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
    printf("pid of proc is %d\n", proc->pcb->pid);
    // printf("%d\n", sig);
    switch(sig) {
        case S_SIGTERM:
            // printf("SIGTERM \n");
            // printf("%d\n", proc->pcb->numChild);
            return k_process_kill(proc, S_SIGTERM);

        case S_SIGCONT:
            printf("SIGCONT\n");
            return 0;

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

