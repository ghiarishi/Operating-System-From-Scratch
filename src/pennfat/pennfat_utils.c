#include "pennfat_utils.h"

/**
 * Reads a line of user input from the prompt and tokenizes it.
 * If the user input is terminated by an EOF, prints a newline.
 * If the user input is *only* an EOF, exits the program with success.
 *
 * This implementation mostly copied from Andrew's hw0.
 *
 * @warning DO NOT USE THIS FOR THE SHELL IMPL! This reads from the host OS' stdin and ignores implementation details
 *  like terminal control. This should be used for the pennfat standalone **only**.
 *
 * @return A pointer to an array of strings, terminated by a NULL element. {NULL} if the user input is all whitespace.
 */
char **readline_tok() {
    // read up to 4096 (max line len) characters from the terminal into a buf
    const int max_line_len = 4096;
    char cmd[max_line_len];
    const char *whitespace = " \t\n";
    char *token;

    ssize_t n_bytes = read(STDIN_FILENO, cmd, max_line_len - 1);
    if (n_bytes == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }
    cmd[n_bytes] = '\0'; // terminate string with a null byte to prevent junk reads

    // exit if ctrl-D on start of line
    if (n_bytes == 0) {
        exit(EXIT_SUCCESS);
    }
    // print newline if read content does not end with newline (i.e. EOF)
    if (cmd[n_bytes - 1] != '\n') {
        fprintf(stderr, "\n");
    }

    // parse the arguments
    // first, alloc at least enough space for the commands
    char **tokens = malloc((n_bytes + 1) * sizeof(char *)); // +1 for the NULL pointer @ end of arg array
    if (tokens == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    // cool pattern from strtok man page!
    // then copy the string pointers into the tokens array
    int n_tokens = 0;
    for (token = strtok(cmd, whitespace);
         token;
         token = strtok(NULL, whitespace)) {
        char *newtok = malloc(sizeof(token) * (strlen(token) + 1));
        strcpy(newtok, token);
        tokens[n_tokens] = newtok;
        n_tokens++;
    }
    // and terminate it with a null pointer
    tokens[n_tokens] = NULL;
    return tokens;
}

/**
 * Utility to free the structure returned by readline_tok().
 * @param toks An array of pointers to heap-allocated strings, terminated by a NULL pointer.
 */
void free_tok(char **toks) {
    for (int i = 0; toks[i] != NULL; ++i) {
        free(toks[i]);
    }
    free(toks);
}

/**
 * Returns the number of tokens in a token array returned by readline_tok().
 */
int toklen(char **toks) {
    int i = 0;
    while (toks[i] != NULL) i++;
    return i;
}