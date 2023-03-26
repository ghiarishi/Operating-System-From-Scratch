#include "process/scheduler.h"
#include "process/pcb.h"
#include "fs/user.h"

// // global:
// fs_t *fs;

// struct Process *highQhead = NULL;
// struct Process *highQtail = NULL;
// struct Process *medQhead = NULL;
// struct Process *medQtail = NULL;
// struct Process *lowQhead = NULL; 
// struct Process *lowQtail = NULL;

// struct Process* highQhead

void testFunc3(){
    printf("helllloooo inside f");
}

int main(int argc, char** argv) {
    printf("main\n");
    // if (argc < 2) {
    //     printf("error");
    // } 
    // char *path = argv[1];
    signal(SIGINT, SIG_IGN); // Ctrl-C
    signal(SIGQUIT, SIG_IGN); /* Ctrl-\ */
    signal(SIGTSTP, SIG_IGN); // Ctrl-Z

    char *argsv[] = {"echo", "hello", "world"};
    int pidNew = p_spawn(testFunc3, argsv, STDIN_FILENO, STDOUT_FILENO);
    printf("%d\n", pidNew);

    return 0;
}