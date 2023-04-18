#include "user_functions.h"

#define cmd_abort(msg) do { f_write(PSTDOUT_FILENO, msg, strlen(msg)); return; } while(0)

void echoFunc(int argc, char *argv[]) {
    // printf("inside echoFunc() \n");
    for (int i = 1; i < argc; i++) {
        char argbuf[strlen(argv[i]) + 1];
        sprintf(argbuf, "%s ", argv[i]);
        f_write(PSTDOUT_FILENO, argbuf, strlen(argv[i]) + 1);
        // todo error checking
    }
}

void sleepFunc(char *argv[]) {
    // int OGticksLeft = ticksLeft;
    dequeue(activeProcess);
    int ticksLeft = 10*atoi(argv[1]);
    printf("num of ticks left %d\n", ticksLeft);
    activeProcess->pcb->sleep_time_remaining = ticksLeft;
    enqueueBlocked(activeProcess);
    
    swapcontext(activeContext, &schedulerContext);
    printf("Back in sleepFunc\n");
    
}

void busy_wait(void) {
    while (1) {
        // Do nothing and keep looping indefinitely
    }
}

void idleFunc(){
    while(1){
        // do nothing
    }
}

void touch(int argc, char* argv[]) {
    if (argc < 2)
        cmd_abort("Usage: touch FILE ...\n");
    // open each file, then write 0 bytes to end and close
    for (int i = 1; i < argc; ++i) {
        int fd = f_open(argv[i], F_APPEND);
        if (fd == -1) {
            p_perror("f_open");
            continue;
        }
        // write 0 bytes to end to update modified time
        if (f_write(fd, NULL, 0) == -1)
            p_perror("f_write");
        f_close( fd);
    }
}

