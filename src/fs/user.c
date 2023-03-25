#include "user.h"

// begin demo code - delete this later
#define MAX_FILES 512

// global:
fs_t *fs;

// inside the pcb:
file_t *fd_table[MAX_FILES];

void this_would_be_main(int argc, char **argv) {
    if (argc < 2) {} // raise some error
    char *path = argv[1];
    fs = fs_mount(path);
    if (fs == NULL) {
        p_perror("fs_mount");
        exit(EXIT_FAILURE);
    }
    // we have a fs
    // the user runs a command that spawns a process that opens some file




}

void init_process() {
    // set up pcb...
    // init fd_table to all NULL, *except* fd_table[PSTDIN_FILENO] and fd_table[PSTDOUT_FILENO]
    bzero(fd_table, sizeof(file_t *) * MAX_FILES);
    file_t *special_stdin_file = malloc(sizeof(file_t));
    special_stdin_file->stdiomode = FIO_STDIN;
    file_t *special_stdout_file = malloc(sizeof(file_t));
    special_stdout_file->stdiomode = FIO_STDOUT;
    fd_table[PSTDIN_FILENO] = special_stdin_file;
    fd_table[PSTDOUT_FILENO] = special_stdout_file;

    // set up ucontext...
    // schedule process...
}

void cleanup_process() {
    // don't forget to free special
}
// end demo code

// user function implementation code
// todo copy all these into the header
int f_open(const char *fname, int mode) {
    file_t *file = fs_open(fs, fname, mode);
    // todo: error checking
    for (int i = 0; i < MAX_FILES; ++i) {
        if (fd_table[i] == NULL) {
            fd_table[i] = file;
            return i;
        }
    }

    // return -1 some with some error code
    ERRNO = PETOOMANYF;
    return -1;
}

int f_close(int fd) {
    file_t *f = fd_table[fd];
    int retval = fs_close(fs, f);
    // todo error checking
    fd_table[fd] = NULL;
    return retval;
}

int f_read(int fd, int n, char *buf) {
    file_t *f = fd_table[fd];
    // todo error checking
    // handle in
    if (f->stdiomode == FIO_STDIN) {
        // write to stdout
    } else if (f->stdiomode == FIO_STDOUT) {
        // todo error, can't read from stdout
    } else {
        return fs_read(fs, f, n, buf);
    }
}


int f_write(int fd, const char *str, int n) {
    // get the file_t * to give to the fs_* method
    file_t *f = fd_table[fd];
    // todo error checking

    // handle stdout
    if (f->stdiomode == FIO_STDOUT) {
        // write to stdout
    } else if (f->stdiomode == FIO_STDIN) {
        // todo error, can't write to stdin
    } else {
        return fs_write(fs, f, str, n);
    }
}