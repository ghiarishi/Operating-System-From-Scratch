#include "user_functions.h"

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