#include "user_functions.h"
#include <time.h>


void echoFunc(int argc, char *argv[]) {
    int i;
    char *message = "";

    // Concatenate all arguments into a single message string
    for (i = 1; i < argc; i++) {
        message = strcat(message, argv[i]);
        if (i < argc - 1) {
            message = strcat(message, " ");
        }
    }

    // Print the message string to stdout
    printf("%s\n", message);
    
}


void sleepFunc(int milliseconds) {
    clock_t start_time = clock();
    while ((clock() - start_time) * 1000 / CLOCKS_PER_SEC < milliseconds) {
        // do nothing
        }
}