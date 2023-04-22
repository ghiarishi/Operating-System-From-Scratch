#include "user.h"
#include "../process/pcb.h"  // included here to avoid circular dependency

/**
 * Open a file. If the file is opened in F_WRITE or F_APPEND mode, the file is created if it does not
 * exist.
 * @param name the name of the file to open
 * @param mode the mode to open the file in (F_WRITE, F_READ, or F_APPEND).
 * @return the file descriptor of the opened file, -1 on error
 * @throw PENOFILE the requested file was in read mode and does not exist
 * @throw PEHOSTIO failed to read from/write to host filesystem
 * @throw PETOOFAT the operation would make a new file but the filesystem is full
 * @throw PEFNAME the operation would make a new file but the filename is invalid
 * @throw PEINUSE the requested file was opened in an exclusive mode and is currently in use
 */
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

/**
 * Closes the specified file, freeing any associated memory.
 * @param fd the file to close
 * @return 0 on a success, -1 on error.
 * @throw PEINVAL the file descriptor is invalid
 */
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

/**
 * Read up to `n` bytes from the specified file into `buf`.
 * @param fd the file to read from
 * @param n the maximum number of bytes to read
 * @param buf a buffer to store the read bytes
 * @return the number of bytes read; -1 on error
 * @throw PEFPERM you do not have permission to read this file
 * @throw PEHOSTIO failed to read from host filesystem
 */
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

/**
 * Write up to `n` bytes from `buf` into the specified file.
 * @param fd the file to write to
 * @param str a buffer storing the bytes to write
 * @param b the maximum number of bytes to write
 * @return the number of bytes written; -1 on error
 * @throw PEFMODE the file is not in write or append mode
 * @throw PEFPERM you do not have permission to write to this file
 * @throw PEHOSTIO failed to read from host filesystem
 * @throw PETOOFAT filesystem is full
 */
ssize_t f_write(int fd, const char *str, ssize_t n) {
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

/**
 * Removes the file with the given name.
 * @param fname the name of the file to delete
 * @return 0 on success; -1 on error
 * @throw PENOFILE the specified file does not exist
 * @throw PEHOSTIO failed to i/o with the host entry
 */
int f_unlink(const char *fname) {
    return fs_unlink(fs, fname);
}

/**
 * Seek the file offset to the given position, given by `offset`. If `whence` is `F_SEEK_SET` this is relative to the
 * start of the file, for `F_SEEK_CUR` relative to the current position, and for `F_SEEK_END` relative to the end of
 * the file.
 * @param fd the file to seek
 * @param offset where to seek to relative to `whence`
 * @param whence the seek mode
 * @return the new location in bytes from start of file; -1 on error
 * @throw PEINVAL whence is not a valid option
 */
uint32_t f_lseek(int fd, int offset, int whence) {
    // get the file_t * to give to the fs_* method
    file_t *f = activeProcess->pcb->fd_table[fd];
    if (f->stdiomode != FIO_NONE)
        return 0;
    return fs_lseek(fs, f, offset, whence);
}

/**
 * Gets information for a file. If `fname` is NULL, gets information for all the files.
 * It is the caller's responsibility to free each of the returned structs. Use the convenience function `f_freels()` to
 * do this quickly.
 * @param fname the name of the file to get the stat of, or NULL to list all files
 * @return a pointer to an array of filestat struct pointers. The array will always be terminated with a NULL pointer.
 * @throw PEHOSTIO failed to read from host filesystem
 */
filestat_t **f_ls(const char *fname) {
    return fs_ls(fs, fname);
}

/**
 * Free the filestat list returned by `f_ls()`.
 */
void f_freels(filestat_t **stat) {
    fs_freels(stat);
}

/**
 * Rename a file.
 * @param oldname the old name
 * @param newname the new name
 * @return 0 on success, -1 on failure
 * @throw PENOFILE the file does not exist
 * @throw PEHOSTIO failed to perform IO on host drive
 * @throw PEFNAME the new name is invalid
 */
int f_rename(const char *oldname, const char *newname) {
    return fs_rename(fs, oldname, newname);
}

/**
 * Edit the I/O permissions of an open file.
 * @param fd the file whose permissions to edit
 * @param mode the mode to edit it in; '+', '=', or '-'
 * @param bitset the permission bitset to edit by (a combination of FAT_EXECUTE, FAT_WRITE, and FAT_READ)
 * @return 0 on success, -1 on error
 * @throw PEINVAL the mode is invalid
 */
int f_chmod(int fd, char mode, uint8_t bitset) {
    file_t *f = activeProcess->pcb->fd_table[fd];
    if (mode == '+')
        f->entry->perm |= bitset;
    else if (mode == '=')
        f->entry->perm = bitset;
    else if (mode == '-')
        f->entry->perm &= ~bitset;
    else
        p_raise(PEINVAL);
    return 0;
}