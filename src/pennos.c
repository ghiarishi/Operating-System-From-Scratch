#include "process/user.h"
#include "process/user_functions.h"
#include "process/shell.h"
#include "fs/user.h"

// global fs - you can move this if needed
fs_t *fs;

void setTimerAlarmHandler(void) {
    
    struct sigaction act;

    act.sa_handler = alarmHandler;
    act.sa_flags = SA_RESTART;
    sigfillset(&act.sa_mask);

    sigaction(SIGALRM, &act, NULL);
}

int main(int argc, char** argv) {
    // fflush(stdin);
    //  if (argc < 2) {
    //      printf("error");
    //  }
    //  char *path = argv[1];
    // fs = fs_mount(path);
    // signal(SIGINT, SIG_IGN); // Ctrl-C
    signal(SIGQUIT, SIG_IGN); /* Ctrl-\ */
    signal(SIGTSTP, SIG_IGN); // Ctrl-Z

    initContext();

    activeProcess = (Process*) malloc(sizeof(Process));
    activeProcess->pcb = initPCB();
    activeProcess->next = NULL;

    pid_t pidNew = p_spawn(pennShell, argv, STDIN_FILENO, STDOUT_FILENO);
    activeProcess->pcb->pid = pidNew;
    
    setSignalHandler();
    setTimer();

    setcontext(&schedulerContext);

    // fprintf(stderr, "Back in the main context\n");

    // freeStacks();

    return 0;
}