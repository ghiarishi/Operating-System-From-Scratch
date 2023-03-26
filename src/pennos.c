#include <stdio.h>
// #include "pcb.h"
#include "scheduler.h"

// // global:
// fs_t *fs;

int main(int argc, char** argv) {
    printf("main\n");
    // if (argc < 2) {
    //     printf("error");
    // } 
    // char *path = argv[1];
    signal(SIGINT, SIG_IGN); // Ctrl-C
    signal(SIGQUIT, SIG_IGN); /* Ctrl-\ */
    signal(SIGTSTP, SIG_IGN); // Ctrl-Z

    struct Process* test_process = createNewProcess(500, -1);
    printf("%s\n", test_process->pcb->argument);
    free(test_process);
    
    return 0;
}

