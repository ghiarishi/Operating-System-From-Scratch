#include "process/scheduler.h"
#include "process/pcb.h"
#include "process/user.h"
#include "process/user_functions.h"
#include "process/shell.h"

void setTimerAlarmHandler(void) {
    
    struct sigaction act;

    act.sa_handler = alarmHandler;
    act.sa_flags = SA_RESTART;
    sigfillset(&act.sa_mask);

    sigaction(SIGALRM, &act, NULL);
}

// void setIntSignalHandler(void) {
//     struct sigaction sa_int;

//     sa_int.sa_handler = sigintHandler;
//     sa_int.sa_flags = SA_RESTART;
//     sigfillset(&sa_int.sa_mask);

//     sigaction(SIGINT, &sa_int, NULL);
// }

// void setTermSignalHandler(void) {
//     struct sigaction sa_term;

//     sa_term.sa_handler = sigtermHandler;
//     sa_term.sa_flags = SA_RESTART;
//     sigfillset(&sa_term.sa_mask);

//     sigaction(SIGTERM, &sa_term, NULL);
// }

// void setContSignalHandler(void) {
//     struct sigaction sa_cont;

//     sa_cont.sa_handler = sigcontHandler;
//     sa_cont.sa_flags = SA_RESTART;
//     sigfillset(&sa_cont.sa_mask);

//     sigaction(SIGALRM, &sa_cont, NULL);
// }

// void setStopSignalmHandler(void) {
//     struct sigaction sa_stop;

//     sa_stop.sa_handler = sigstopHandler;
//     sa_stop.sa_flags = SA_RESTART;
//     sigfillset(&sa_cont.sa_mask);

//     sigaction(SIGALRM, &sa_cont, NULL);
// }

int main(int argc, char** argv) {
    fflush(stdin);
    // if (argc < 2) {
    //     printf("error");
    // } 
    // char *path = argv[1];
    // signal(SIGINT, SIG_IGN); // Ctrl-C
    signal(SIGQUIT, SIG_IGN); /* Ctrl-\ */
    signal(SIGTSTP, SIG_IGN); // Ctrl-Z

    initContext();

    activeProcess = (Process*) malloc(sizeof(Process));
    activeProcess->pcb = initPCB();
    activeProcess->next = NULL;

    pid_t pidNew = p_spawn(pennShell, argv, STDIN_FILENO, STDOUT_FILENO);
    printf("printing pid %d\n", pidNew);
    // activeProcess->pcb->pid = pidNew;
    // pennShell();
    
    setTimerAlarmHandler();
    // alarm(5);
    setTimer();

    setcontext(&schedulerContext);

    // fprintf(stderr, "Back in the main context\n");

    // freeStacks();

    return 0;
}