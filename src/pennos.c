#include "process/scheduler.h"
#include "process/pcb.h"
#include "process/user.h"
#include "process/user_functions.h"
#include "process/shell.h"

int main(int argc, char** argv) {
    printf("main \n");
    // if (argc < 2) {
    //     printf("error");
    // } 
    // char *path = argv[1];
    // signal(SIGINT, SIG_IGN); // Ctrl-C
    signal(SIGQUIT, SIG_IGN); /* Ctrl-\ */
    signal(SIGTSTP, SIG_IGN); // Ctrl-Z

    initSchedulerContext();

    activeProcess = (Process*) malloc(sizeof(Process));
    activeProcess->pcb = initPCB();
    activeProcess->next = NULL;

    pid_t pidNew = p_spawn(pennShell, argv, STDIN_FILENO, STDOUT_FILENO);
    printf("printing pid %d\n", pidNew);
    // activeProcess->pcb->pid = pidNew;
    // pennShell();
    
    setAlarmHandler();
    // alarm(5);
    setTimer();

    setcontext(&schedulerContext);

    // fprintf(stderr, "Back in the main context\n");

    // freeStacks();

    return 0;
}