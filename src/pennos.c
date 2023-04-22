#include "process/user.h"
#include "process/user_functions.h"
#include "process/shell.h"
#include "fs/user.h"
#include "process/dependencies.h"

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
    // mount global filesystem
    
    // char file[] = "log";
    if (argc < 2) {
        // fprintf(fp, "Usage: %s <filesystem>\n", argv[0]);
        // exit(EXIT_FAILURE);
        p_perror("invalid");
        p_exit();
    }
<<<<<<< Updated upstream
    // else if (argc == 2){
        
    //     FILE *fp = fopen(file, "w");
    //     fprintf(fp, "Hello, world!\n");
    //     fclose(fp);
    // }
    // else if (argc ==3){
    //     FILE *fp = fopen(argv[2], "w");
    //     fprintf(fp, "Hello, world!\n");
    //     // fclose(fp);
    // }
=======
    else if (argc == 2){
        // file = "log";
        shellargs=2;
        FILE *fp = fopen("logs", "w");
        fprintf(fp, "PennOS Logs\n");
        fclose(fp);
    }
    else if (argc ==3){
        // file = argv[2];
        shellargs=3;
        FILE *fp = fopen(argv[2], "w");
        fprintf(fp, "PennOS Logs\n");
        fclose(fp);
    }
>>>>>>> Stashed changes
    char *path = argv[1];
    fs = fs_mount(path);
    if (fs == NULL) {
        p_perror("fs_mount");
        // exit(EXIT_FAILURE);
        p_exit();
    }
    // signal(SIGINT, SIG_IGN); // Ctrl-C
    signal(SIGQUIT, SIG_IGN); /* Ctrl-\ */
    
    // signal(SIGTSTP, SIG_IGN); // Ctrl-Z

    initContext();

    activeProcess = (Process*) malloc(sizeof(Process));
    activeProcess->pcb = initPCB();
    activeProcess->next = NULL;

    char* argv1 = malloc(1 * sizeof("penn-shell"));
    strcpy(argv1, "penn-shell");
    char* argvNew[] = {argv1, NULL};

    // check if can be replaced by k proc create (avoids need for if, and diff priority)
    
    pid_t pidNew = p_spawn(pennShell, argvNew, STDIN_FILENO, STDOUT_FILENO);

    activeProcess->pcb->pid = pidNew;

    setSignalHandler();
    setTimer();

    setcontext(&schedulerContext);

    // fprintf(stderr, "Back in the main context\n");

    // freeStacks();
    // fclose(fp);
    return 0;
}