#include "user.h"

// static ucontext_t schedulerContext;

pid_t p_spawn(void (*func)(), char *argv[], int fd0, int fd1) {

    printf("Inside p_spawn \n");
    pid_t pid_new, pid_new1;
    pid_t pid = getpid();

    // size_t size = BUFFERSIZE;
    // while(1){
    //     if (write(STDERR_FILENO, PROMPT, sizeof(PROMPT)) == -1){ // write the command prompt
    //         perror("Unable to write");
    //     }

    //     char *input = malloc(sizeof(char) * BUFFERSIZE);
    //     int pos = 0;
    //     pos = getline(&input, &size, stdin);

    //     printf("%s", input);
    // }
   
    struct Process *testProcess = createNewProcess(echoFunc, argv, pid, -1);
    struct Process *testProcess1 = createNewProcess(sleepFunc, argv, pid, 0);
    pid_new = testProcess->pcb->pid;
    pid_new1 = testProcess1->pcb->pid;
    printf("passed by make context %d \n", pid_new1);
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