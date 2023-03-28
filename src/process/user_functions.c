#include "user_functions.h"

void echoFunc(int argc, char *argv[]) {
    int i;
    char *message = "";
    // Concatenate all arguments into a single message string
    printf("running echo!");
    for (i = 1; i < argc; i++) {
        message = strcat(message, argv[i]);
        if (i < argc - 1) {
            message = strcat(message, " ");
        }
    }
    // Print the message string to stdout
    printf("%s \n", message);
    swapcontext(activeContext, &schedulerContext);
}

void sleepFunc(int milliseconds) {
    milliseconds = 12000;
    clock_t start_time = clock();
    printf("running sleep! \n");
    while ((clock() - start_time) * 1000 / CLOCKS_PER_SEC < milliseconds) {
        // do nothing
    }
    swapcontext(&mainContext, &schedulerContext);
}