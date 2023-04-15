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

// void signalHandler(int signum) {

//     switch
// }

// void setSignalHandler(void) {

//     struct sigaction sa_alarm;

//     sa_alarm.sa_handler = alarmHandler;
//     sa_alarm.sa_flags = SA_RESTART;
//     sigfillset(&sa_alarm.sa_mask);

//     sigaction(SIGALRM, &sa_alarm, NULL);

//     struct sigaction sa_int;

//     sa_int.sa_handler = signalHandler;
//     sa_int.sa_flags = SA_RESTART;
//     sigfillset(&sa_int.sa_mask);

//     sigaction(SIGINT, &sa_int, NULL);

//     struct sigaction sa_term;

//     sa_term.sa_handler = signalHandler;
//     sa_term.sa_flags = SA_RESTART;
//     sigfillset(&sa_term.sa_mask);

//     sigaction(S_SIGTERM, &sa_term, NULL);

//     struct sigaction sa_cont;

//     sa_cont.sa_handler = signalHandler;
//     sa_cont.sa_flags = SA_RESTART;
//     sigfillset(&sa_cont.sa_mask);

//     sigaction(S_SIGCONT, &sa_cont, NULL);

//     struct sigaction sa_stop;

//     sa_stop.sa_handler = signalHandler;
//     sa_stop.sa_flags = SA_RESTART;
//     sigfillset(&sa_cont.sa_mask);

//     sigaction(S_SIGTSTOP, &sa_stop, NULL);
// }

int main(int argc, char** argv) {
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
    
    setTimerAlarmHandler();
    // alarm(5);
    setTimer();

    setcontext(&schedulerContext);

    // fprintf(stderr, "Back in the main context\n");

    // freeStacks();

    return 0;
}