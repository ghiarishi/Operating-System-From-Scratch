#include "user.h"
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

    Process *newProcess = (Process*) malloc(sizeof(Process)); //NULL;
    newProcess->pcb = k_process_create(activeProcess->pcb);
    pid_new = newProcess->pcb->pid;

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



// pid_t p_waitpid(pid_t pid, int *wstatus, bool nohang) {
//     // parent p_spwans child. Child runs. OS tracks child. Child complete/stopped. 
//     // MAYBE:
//     // OS sends signal to parent. Parent has sig handler. 
//     // There's a zombie queue. WHERE DOES IT COME INTO THE PICTURE??
//     // When child completes running, it goes to the zombie queue. MAYBE.
//     // LOTS OF MAYBES TANVI FIGURE IT OUT.
// }

// pid_t p_waitpid(pid_t pid, int *wstatus, bool nohang) {
//     Process *p;

//     // Find the child process with the specified pid
//     // p = findProcessByPid(pid);

//     // If the process does not exist or is not a child of the current process, return error
//     // if (p == NULL || p->parent != getCurrentProcess()) {
//     //     return -1;
//     // }

//     // Block the current process if nohang is false
//     if (!nohang) {
//         blockProcess(getCurrentProcess(), WAITING);
//     }

//     // Wait for the child process to change state
//     while (p->state != TERMINATED) {
//         yield();
//     }

//     // Return the child pid on success
//     if (wstatus != NULL) {
//         *wstatus = p->exitStatus;
//     }
//     return p->pid;
// }

void p_sleep(unsigned int ticks){
    // sleep 
    printf("inside p_sleep, hello \n");

}

// int p_kill(pid_t pid, int sig){
//     //Killing should free all memory locations. Remove it from all queues. 
//     // Sig should come from parent? 
//     // Kill the process with the pid passed => What does killing mean here? 
//     // Only one pid essentially. We create a "process". Gets a pcb. calls funcs. So you clear memory. And check if anything referenced in parent??

// }

// void p_exit(void){
//     //should be like killing but don't care about signal. just KILL KILL KILL.
// }