#include "user.h"
#include "../process/pcb.h"  // included here to avoid circular dependency

Process *activeProcess;

// begin demo code - delete this later

// global:
// fs_t *fs;

// inside the pcb:
// file_t *fd_table[MAX_FILES];

// void this_would_be_main(int argc, char **argv) {
//     if (argc < 2) {} // p_raise some error
//     char *path = argv[1];
//     fs = fs_mount(path);
//     if (fs == NULL) {
//         p_perror("fs_mount");
//         exit(EXIT_FAILURE);
//     }
//     // we have a fs
//     // the user runs a command that spawns a process that opens some file

// }

// void init_process() {
//     // set up pcb...
//     // init fd_table to all NULL, *except* fd_table[PSTDIN_FILENO] and fd_table[PSTDOUT_FILENO]
//     bzero(fd_table, sizeof(file_t *) * MAX_FILES);
//     file_t *special_stdin_file = malloc(sizeof(file_t));
//     special_stdin_file->stdiomode = FIO_STDIN;
//     file_t *special_stdout_file = malloc(sizeof(file_t));
//     special_stdout_file->stdiomode = FIO_STDOUT;
//     fd_table[PSTDIN_FILENO] = special_stdin_file;
//     fd_table[PSTDOUT_FILENO] = special_stdout_file;

//     // set up ucontext...
//     // schedule process...
// }

// void cleanup_process() {
//     // don't forget to free special
// }
// // end demo code



// user function implementation code
int f_open(const char *fname, int mode) {
    file_t *file = fs_open(fs, fname, mode);
    // error checking
    if (file == NULL)
        return -1;

    for (int i = 0; i < MAX_FILES; ++i) {
        if (activeProcess->pcb->fd_table[i] == NULL) {
            activeProcess->pcb->fd_table[i] = file;
            return i;
        }
    }

    // return -1 some with some error code
    ERRNO = PETOOMANYF;
    return -1;
}

int f_close(int fd) {
    file_t *f = activeProcess->pcb->fd_table[fd];
    int retval;
    if (f->stdiomode == FIO_NONE) {
        retval = fs_close(fs, f);
    } else {
        free(f);
        retval = 0;
    }
    activeProcess->pcb->fd_table[fd] = NULL;
    return retval;
}

ssize_t f_read(int fd, int n, char *buf) {
    file_t *f = activeProcess->pcb->fd_table[fd];
    // handle stdin
    if (f->stdiomode == FIO_STDIN) {
        // read from host stdin
        ssize_t host_retval = read(STDIN_FILENO, buf, n);
        if (host_retval == -1)
            p_raise(PEHOSTIO);
        return host_retval;
    } else if (f->stdiomode == FIO_STDOUT) {
        p_raise(PESTDIO);
    } else {
        return fs_read(fs, f, n, buf);
    }
}


ssize_t f_write(int fd, const char *str, int n) {
    // get the file_t * to give to the fs_* method
    file_t *f = activeProcess->pcb->fd_table[fd];
    // handle stdout
    if (f->stdiomode == FIO_STDOUT) {
        // write to host stdout
        ssize_t host_retval = write(STDOUT_FILENO, str, n);
        if (host_retval == -1)
            p_raise(PEHOSTIO);
        return host_retval;
    } else if (f->stdiomode == FIO_STDIN) {
        p_raise(PESTDIO);
    } else {
        return fs_write(fs, f, str, n);
    }
}

int f_unlink(const char *fname) {
    return fs_unlink(fs, fname);
}

uint32_t f_lseek(int fd, int offset, int whence) {
    // get the file_t * to give to the fs_* method
    file_t *f = activeProcess->pcb->fd_table[fd];
    if (f->stdiomode != FIO_NONE)
        return 0;
    return fs_lseek(fs, f, offset, whence);
}

filestat_t **f_ls(const char *fname) {
    return fs_ls(fs, fname);
}

void f_freels(filestat_t **stat) {
    fs_freels(stat);
}
