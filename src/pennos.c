#include "process/scheduler.h"
#include "process/pcb.h"
#include "process/user.h"
#include "process/user_functions.h"

// // global:
// fs_t *fs;

// struct Process *highQhead = NULL;
// struct Process *highQtail = NULL;
// struct Process *medQhead = NULL;
// struct Process *medQtail = NULL;
// struct Process *lowQhead = NULL; 
// struct Process *lowQtail = NULL;

// struct Process* highQhead

int main(int argc, char** argv) {
    printf("main\n");
    // if (argc < 2) {
    //     printf("error");
    // } 
    // char *path = argv[1];
    // signal(SIGINT, SIG_IGN); // Ctrl-C
    signal(SIGQUIT, SIG_IGN); /* Ctrl-\ */
    signal(SIGTSTP, SIG_IGN); // Ctrl-Z

    initSchedulerContext();

    char *argsv[] = {"echo", "hello", "world"};
    pid_t pidNew = p_spawn(echoFunc, argsv, STDIN_FILENO, STDOUT_FILENO);
    printf("%d \n", pidNew);

    setAlarmHandler();
    alarm(5);
    // setTimer();

    swapcontext(&mainContext, &schedulerContext);

    fprintf(stderr, "Back in the main context\n");

    freeStacks();

    return 0;
}