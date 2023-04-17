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

// demo code showing high-level redirection logic
//void foo() {
//    int infd = f_open("...", F_READ);
//    int outfd = f_open("...", F_WRITE);
//    p_spawn(NULL, NULL, infd, outfd);
//    f_close(infd);
//    f_close(outfd);
//}

pid_t p_spawn(void (*func)(), char *argv[], int fd0, int fd1) {

    // printf("Inside p_spawn \n");
    pid_t pid_new;

    // pid_t pid = getpid();

    Process *newProcess = (Process*) malloc(sizeof(Process)); //NULL;
    newProcess->pcb = k_process_create(activeProcess->pcb);
    pid_new = newProcess->pcb->pid;

    // set up child fd table based on fd0/fd1
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

    struct parsed_command *cmd;
    parse_command(concat(argc, argv), &cmd);

    newProcess->pcb->argument = malloc((strlen(concat(argc, argv))+1) * sizeof(char));
    newProcess->pcb->argument = concat(argc, argv);

    // printf("The arguments for new Process %c\n", *cmd->commands[0][1]);

    if(func == (void(*)())&pennShell){
        newProcess->pcb->priority = -1;
        makecontext(&newProcess->pcb->context, func, 0, argv);

        // printf("making pennshell context \n");
    }
    else if(func == (void(*)())&sleepFunc){
        makecontext(&newProcess->pcb->context, func, 2, cmd->commands[0]);
    }
    else if(func == (void(*)())&echoFunc){
        makecontext(&newProcess->pcb->context, func, 2, argc, cmd->commands[0]);
    }
    printf("newProcess->pid: %d\n", newProcess->pcb->pid);
    // printf("enqueuing now!\n");
    enqueue(newProcess);

    return pid_new;
}

// void p_sleep(unsigned int ticks){
//     // sleep 
//     printf("inside p_sleep, hello \n");

// }

pid_t p_waitpid(pid_t pid, int *wstatus, bool nohang) {

    // Find the child process with the specified pid
    Process *p = findProcessByPid(pid);

    printf("inside waitpid child%d; parent%d\n", p->pcb->pid, activeProcess->pcb->pid);

    activeProcess->pcb->waitChild = pid;

    if(!nohang){
        dequeue(activeProcess);
        enqueueBlocked(activeProcess);
        swapcontext(activeContext, &schedulerContext);
    }

    wstatus = &p->pcb->status;

    int tpid = p->pcb->pid;

    free(p);
    freeStacks(p->pcb);
    freePcb(p->pcb);

    return tpid;
}


int p_kill(pid_t pid, int sig){
    
    switch(sig){
        case S_SIGTERM: 
            // Process *child = NULL;
            // for(int i=0;i<parent->pcb->numChild;i++){
            //     child = findProcessByPid(parent->pcb->childPids[i]);
            //     k_process_cleanup(child, S_SIGTERM);
            // }

            // k_process_cleanup(parent, sig);
            break;

        case S_SIGCONT:
            break;

        case S_SIGTSTP:
            break;
    }
    
    return 1;

}

// void p_exit(void){
//     //should be like killing but don't care about signal. just KILL KILL KILL.
// }