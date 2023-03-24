#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "fs/fat.h"
#include "pennfat/pennfat_utils.h"
#include "common/errno.h"

#define PROMPT "pennfat> "
#define cmd_abort(msg) do { fprintf(stderr, "%s", msg); return; } while(0)

// global: the currently mounted filesystem
fs_t *fs = NULL;

// ==== command impls ====
/**
 * mkfs FS_NAME BLOCKS_IN_FAT BLOCK_SIZE_CONFIG
 * Creates a PennFAT filesystem in the file named FS_NAME. The number of blocks in the FAT region is BLOCKS_IN_FAT
 * (ranging from 1 through 32), and the block size is 256, 512, 1024, 2048, or 4096 bytes corresponding to the value (0
 * through 4) of BLOCK_SIZE_CONFIG.
 */
void mkfs(char **args) {
    if (toklen(args) != 4)
        cmd_abort("Usage: mkfs FS_NAME BLOCKS_IN_FAT BLOCK_SIZE_CONFIG\n");
    char *name = args[1];
    int fat_blocks = atoi(args[2]);
    int fat_config = atoi(args[3]);
    if (!(fat_blocks > 0 && fat_blocks < 33))
        cmd_abort("BLOCKS_IN_FAT must be in the range [1..32]\n");
    int block_size;
    switch (fat_config) {
        case 0:
            block_size = 256;
            break;
        case 1:
            block_size = 512;
            break;
        case 2:
            block_size = 1024;
            break;
        case 3:
            block_size = 2048;
            break;
        case 4:
            block_size = 4096;
            break;
        default:
            cmd_abort("BLOCK_SIZE_CONFIG must be in the range [0..4]\n");
    }

    // open the file to write to, creating it with r/w perms if needed
    int fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("open");
        return;
    }

    // calculate file sizes
    int n_entries = block_size * fat_blocks / 2;
    int fat_size = block_size * fat_blocks;
    int data_size = block_size * (n_entries - 1);
    // special case: if n_entries = 0x10000, cut off the end block (0xffff) since we can't reach it
    if (n_entries == 0x10000) data_size -= block_size;
    // write 0s to the entire file
    if (ftruncate(fd, fat_size + data_size) == -1) {
        perror("ftruncate");
        return;
    }
    // write header = [fat_config][fat_blocks][ff][ff] => config, then root dir EOF (little endian)
    uint8_t header[4] = {fat_config, fat_blocks, 0xff, 0xff};
    if (write(fd, header, 4) == -1) {
        perror("write");
        return;
    }
    // ok!
    close(fd);
}

/**
 * mount FS_NAME
 * Mounts the filesystem named FS_NAME by loading its FAT into memory.
 */
void mount(char **args) {
    if (toklen(args) != 2)
        cmd_abort("Usage: mount FS_NAME\n");
    if (fs != NULL)
        cmd_abort("You have already mounted a filesystem\n");
    fs = fs_mount(args[1]);
    if (fs == NULL)
        p_perror("fs_mount");
}

/**
 * umount
 * Unmounts the currently mounted filesystem.
 */
void umount(char **args) {
    if (toklen(args) != 1)
        cmd_abort("Usage: umount\n");
    if (fs == NULL)
        cmd_abort("You do not have a filesystem mounted\n");
    if (fs_unmount(fs) == -1)
        p_perror("fs_unmount");
    fs = NULL;
}

/**
 * touch FILE ...
 * Creates the files if they do not exist, or updates their timestamp to the current system time
 */
void touch(char **args) {
    int n_args = toklen(args);
    if (n_args < 2)
        cmd_abort("Usage: touch FILE ...\n");
    if (fs == NULL)
        cmd_abort("You do not have a filesystem mounted\n");
    // open each file, then write 0 bytes to end and close
    for (int i = 1; i < n_args; ++i) {
        file_t *f = fs_open(fs, args[i], F_APPEND);
        if (f == NULL) {
            p_perror("fs_open");
            continue;
        }
        // write 0 bytes to end to update modified time
        if (fs_write(fs, f, NULL, 0) == -1)
            p_perror("fs_write");
        fs_close(fs, f);
    }
}

/**
 * mv SOURCE DEST
 * Renames SOURCE to DEST.
 */
void mv(char **args) {
    if (toklen(args) != 3)
        cmd_abort("Usage: mv SOURCE DEST\n");
    if (fs == NULL)
        cmd_abort("You do not have a filesystem mounted\n");
    if (fs_rename(fs, args[1], args[2]) == -1)
        p_perror("fs_rename");
}

/**
 * rm FILE ...
 * Removes the files.
 */
void rm(char **args) {
    int n_args = toklen(args);
    if (n_args < 2)
        cmd_abort("Usage: rm FILE ...\n");
    if (fs == NULL)
        cmd_abort("You do not have a filesystem mounted\n");
    // delete each file
    for (int i = 1; i < n_args; ++i) {
        if (fs_unlink(fs, args[i]) == -1)
            p_perror("fs_unlink");
    }
}

/**
 * cat FILE ... [ -w OUTPUT_FILE ]
 * Concatenates the files and prints them to stdout by default, or overwrites OUTPUT_FILE. If OUTPUT_FILE does not
 * exist, it will be created. (Same for OUTPUT_FILE in the commands below.)
 *
 * cat FILE ... [ -a OUTPUT_FILE ]
 * Concatenates the files and prints them to stdout by default, or appends to OUTPUT_FILE.
 *
 * cat -w OUTPUT_FILE
 * Reads from the terminal and overwrites OUTPUT_FILE.
 *
 * cat -a OUTPUT_FILE
 * Reads from the terminal and appends to OUTPUT_FILE.
 */
void cat(char **args) {
    int n_args = toklen(args);
    if (n_args < 2)
        cmd_abort("Usage: cat [FILE ...] [-w OUTPUT_FILE | -a APPEND_FILE]\n");
    if (fs == NULL)
        cmd_abort("You do not have a filesystem mounted\n");
    // figure out if we are outputting to a file
    char *output_filename;
    int output_mode = 0, n_input_files = 0;
    for (int i = 1; i < n_args; ++i) {
        if (strcmp(args[i], "-w") == 0) {
            if (i != n_args - 2) cmd_abort("Usage: cat [FILE ...] [-w OUTPUT_FILE | -a APPEND_FILE]\n");
            output_mode = F_WRITE;
            output_filename = args[i + 1];
            break;
        }
        if (strcmp(args[i], "-a") == 0) {
            if (i != n_args - 2) cmd_abort("Usage: cat [FILE ...] [-w OUTPUT_FILE | -a APPEND_FILE]\n");
            output_mode = F_APPEND;
            output_filename = args[i + 1];
            break;
        }
        n_input_files++;
    }
    // if we're writing to a file, open it
    file_t *dest;
    if (output_mode) {
        dest = fs_open(fs, output_filename, output_mode);
        if (dest == NULL) {
            p_perror("fs_open");
            return;
        }
    }

    char buf[4097];
    ssize_t bytes_read;

    // stdin
    if (n_input_files == 0) {
        if (output_mode == 0)
            cmd_abort("Usage: cat [FILE ...] [-w OUTPUT_FILE | -a APPEND_FILE]\n");
        // copy from stdin until EOF
        do {
            // read a chunk
            bytes_read = read(STDIN_FILENO, buf, 4096);
            if (bytes_read == -1) {
                perror("read");
                goto cat_end;
            }
            buf[bytes_read] = 0;
            // then write that chunk to the dest
            if (fs_write(fs, dest, buf, bytes_read) != bytes_read) {
                p_perror("fs_write");
                goto cat_end;
            }
        } while (bytes_read);
    } else {
        // open each read file, then read its contents to stdout in 4KB chunks
        for (int i = 1; i <= n_input_files; ++i) {
            file_t *f = fs_open(fs, args[i], F_READ);
            if (f == NULL) {
                p_perror("fs_open");
                goto cat_end;
            }
            // read until there is nothing more
            do {
                // read a chunk
                bytes_read = fs_read(fs, f, 4096, buf);
                if (bytes_read == -1) {
                    p_perror("fs_read");
                    fs_close(fs, f);
                    goto cat_end;
                }
                buf[bytes_read] = 0;
                // then write that chunk to the dest
                if (output_mode) {
                    if (fs_write(fs, dest, buf, bytes_read) != bytes_read) {
                        p_perror("fs_write");
                        fs_close(fs, f);
                        goto cat_end;
                    }
                } else {
                    // if dest is stdout
                    if (write(STDOUT_FILENO, buf, bytes_read) == -1) {
                        perror("write");
                        fs_close(fs, f);
                        goto cat_end;
                    }
                }
            } while (bytes_read);
            fs_close(fs, f);
        }
    }

    cat_end:
    // close dest if it's a file
    if (output_mode)
        fs_close(fs, dest);
}

/**
 * cp [ -h ] SOURCE DEST
 * Copies SOURCE to DEST. With -h, SOURCE is from the host OS.
 *
 * cp SOURCE -h DEST
 * Copies SOURCE from the filesystem to DEST in the host OS.
 */
void cp(char **args) {
    int n_args = toklen(args);
    if (n_args < 3 || n_args > 4)
        cmd_abort("Usage: cp SOURCE -h DEST | cp [-h] SOURCE DEST\n");
    if (fs == NULL)
        cmd_abort("You do not have a filesystem mounted\n");

    char buf[4097];
    ssize_t bytes_read;
    // local copy
    if (n_args == 3) {
        file_t *src = fs_open(fs, args[1], F_READ);
        if (src == NULL) {
            p_perror("fs_open");
            return;
        }
        file_t *dest = fs_open(fs, args[2], F_WRITE);
        if (dest == NULL) {
            fs_close(fs, src);
            p_perror("fs_open");
            return;
        }
        // read until there is nothing more
        do {
            // read a chunk
            bytes_read = fs_read(fs, src, 4096, buf);
            if (bytes_read == -1) {
                p_perror("fs_read");
                break;
            }
            buf[bytes_read] = 0;
            // then write that chunk to the dest
            if (fs_write(fs, dest, buf, bytes_read) != bytes_read) {
                p_perror("fs_write");
                break;
            }
        } while (bytes_read);
        fs_close(fs, src);
        fs_close(fs, dest);
    } else if (strcmp(args[1], "-h") == 0) {
        // from host
        int src = open(args[2], O_RDONLY);
        if (src == -1) {
            p_perror("open");
            return;
        }
        file_t *dest = fs_open(fs, args[3], F_WRITE);
        if (dest == NULL) {
            close(src);
            p_perror("fs_open");
            return;
        }
        // read until there is nothing more
        do {
            // read a chunk
            bytes_read = read(src, buf, 4096);
            if (bytes_read == -1) {
                p_perror("fs_read");
                break;
            }
            buf[bytes_read] = 0;
            // then write that chunk to the dest
            if (fs_write(fs, dest, buf, bytes_read) != bytes_read) {
                p_perror("fs_write");
                break;
            }
        } while (bytes_read);
        close(src);
        fs_close(fs, dest);
    } else if (strcmp(args[2], "-h") == 0) {
        // to host
        file_t *src = fs_open(fs, args[1], F_READ);
        if (src == NULL) {
            p_perror("fs_open");
            return;
        }
        int dest = open(args[3], O_WRONLY | O_TRUNC | O_CREAT, 0666);
        if (dest == -1) {
            fs_close(fs, src);
            p_perror("open");
            return;
        }
        // read until there is nothing more
        do {
            // read a chunk
            bytes_read = fs_read(fs, src, 4096, buf);
            if (bytes_read == -1) {
                p_perror("fs_read");
                break;
            }
            buf[bytes_read] = 0;
            // then write that chunk to the dest
            if (write(dest, buf, bytes_read) != bytes_read) {
                p_perror("fs_write");
                break;
            }
        } while (bytes_read);
        fs_close(fs, src);
        close(dest);
    } else {
        cmd_abort("Usage: cp SOURCE -h DEST | cp [-h] SOURCE DEST\n");
    }
}

/**
 * ls
 * List all files in the directory.
 */
void ls(char **args) {
    if (toklen(args) != 1)
        cmd_abort("Usage: ls\n");
    if (fs == NULL)
        cmd_abort("You do not have a filesystem mounted\n");
    // handy ls!
    filestat_t **entries = fs_ls(fs, NULL);
    for (int i = 0; entries[i] != NULL; ++i) {
        filestat_t *entry = entries[i];
        char x = entry->perm & FAT_EXECUTE ? 'x' : '-';
        char r = entry->perm & FAT_READ ? 'r' : '-';
        char w = entry->perm & FAT_WRITE ? 'w' : '-';
        struct tm *timestruct = localtime(&entry->mtime);
        char date[32];
        strftime(date, 32, "%b %e %H:%M", timestruct);
        // blockno perm size month day time name
        fprintf(stdout, "%d\t%c%c%c\t%d\t%s\t%s\n", entry->blockno, x, r, w, entry->size, date, entry->name);
    }
    fs_freels(entries);
}

/**
 * chmod ([+-=][rwx]*|OCT) FILE ...
 * Similar to chmod(1) in the VM.
 *
 * execute = 1
 * write = 2
 * read = 4
 */
void chmod(char **args) {
    int n_args = toklen(args);
    if (n_args < 3)
        cmd_abort("chmod ([+=-][rwx]*|OCT) FILE ...\n");
    if (fs == NULL)
        cmd_abort("You do not have a filesystem mounted\n");

    uint8_t bitset = 0;
    char mode;
    // parse arg 1: does it start with [+=-]?
    if (args[1][0] == '+' || args[1][0] == '=' || args[1][0] == '-') {
        mode = args[1][0];
        for (int i = 1; i < strlen(args[1]); ++i) {
            if (args[1][i] == 'x')
                bitset |= FAT_EXECUTE;
            else if (args[1][i] == 'w')
                bitset |= FAT_WRITE;
            else if (args[1][i] == 'r')
                bitset |= FAT_READ;
            else
                cmd_abort("mode must be of the form /[+=-][rwx]+/ or in the range [0..7]\n");
        }
    } else {
        // must be a single octal digit then
        if (!isdigit(args[1][0]))
            cmd_abort("mode must be of the form /[+=-][rwx]+/ or in the range [0..7]\n");
        int arg = atoi(args[1]);
        if (arg < 0 || arg > 7)
            cmd_abort("mode must be of the form /[+=-][rwx]+/ or in the range [0..7]\n");
        bitset = arg;
        mode = '=';
    }

    // open each file, then set its permission based on bitset and mode
    for (int i = 2; i < n_args; ++i) {
        file_t *f = fs_open(fs, args[i], F_APPEND);
        if (f == NULL) {
            p_perror("fs_open");
            return;
        }
        if (mode == '+')
            f->entry->perm |= bitset;
        else if (mode == '=')
            f->entry->perm = bitset;
        else // mode == '-'
            f->entry->perm &= ~bitset;
        fs_close(fs, f);
    }
}

// ==== main "shell" impl ====
/**
 * Main loop: prompt, read, execute.
 */
void loop() {
    fprintf(stderr, PROMPT);
    char **args = readline_tok();

    // if there are no args (all WS), return and restart the loop
    if (args[0] == NULL) {
        free_tok(args);
        return;
    }

    // otherwise, run the command
    if (!strcmp(args[0], "mkfs"))
        mkfs(args);
    else if (!strcmp(args[0], "mount"))
        mount(args);
    else if (!strcmp(args[0], "umount"))
        umount(args);
    else if (!strcmp(args[0], "touch"))
        touch(args);
    else if (!strcmp(args[0], "mv"))
        mv(args);
    else if (!strcmp(args[0], "rm"))
        rm(args);
    else if (!strcmp(args[0], "cat"))
        cat(args);
    else if (!strcmp(args[0], "cp"))
        cp(args);
    else if (!strcmp(args[0], "ls"))
        ls(args);
    else if (!strcmp(args[0], "chmod"))
        chmod(args);
    else
        fprintf(stderr, "Unrecognized command: %s\n", args[0]);

    // and clean up
    free_tok(args);
}

int main() {
    // ==== main loop ====
    while (true) {
        loop();
    }
}

