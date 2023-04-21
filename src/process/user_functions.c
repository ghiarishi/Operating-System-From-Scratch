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
    printf("\n");
}

void sleepFunc(int argc, char *argv[]) {
    // printf("entered sleep func \n");
    int ticksLeft = 10*atoi(argv[1]);
    p_sleep(ticksLeft);
    
    // printf("Back in sleepFunc\n");
    
}

void busyFunc(void) {
    while (1) {
        // Do nothing and keep looping indefinitely
    }
}

void idleFunc(){
    while(1){
        // do nothing
    }
}


// ==== filesystem ====

/**
 * cat FILE ...
 * Concatenates the files and prints them to stdout.
 *
 * cat
 * Reads from stdin until EOF and prints to stdout.
 */
void catFunc(int argc, char **argv) {
    char buf[4096];
    ssize_t bytes_read;

    // stdin
    if (argc == 1) {
        // copy from stdin until EOF
        do {
            // read a chunk
            bytes_read = f_read(PSTDIN_FILENO,  4096, buf);
            if (bytes_read == -1) {
                p_perror("f_read");
                return;
            }
            // then write that chunk to the dest
            if (f_write(PSTDOUT_FILENO, buf, bytes_read) != bytes_read) {
                p_perror("f_write");
                return;
            }
        } while (bytes_read);
    } else {
        // open each read file, then read its contents to stdout in 4KB chunks
        for (int i = 1; i < argc; ++i) {
            int fd = f_open(argv[i], F_READ);
            if (fd == -1) {
                p_perror("f_open");
                return;
            }
            // read until there is nothing more
            do {
                // read a chunk
                bytes_read = f_read(fd, 4096, buf);
                if (bytes_read == -1) {
                    p_perror("f_read");
                    f_close(fd);
                    return;
                }
                // write to stdout
                if (f_write(PSTDOUT_FILENO, buf, bytes_read) != bytes_read) {
                    p_perror("f_write");
                    f_close(fd);
                    return;
                }
            } while (bytes_read);
            f_close(fd);
        }
    }
}

/**
 * ls
 * List all files in the directory.
 */
void lsFunc(int argc, char **argv) {
    if (argc != 1)
        cmd_abort("Usage: ls\n");
    // handy ls!
    filestat_t **entries = f_ls(NULL);
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
    f_freels(entries);
}

/**
 * touch FILE ...
 * Creates the files if they do not exist, or updates their timestamp to the current system time
 */
void touchFunc(int argc, char **argv) {
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
        f_close(fd);
    }
}

/**
 * mv SOURCE DEST
 * Renames SOURCE to DEST.
 */
void mvFunc(int argc, char **argv) {
    if (argc != 3)
        cmd_abort("Usage: mv SOURCE DEST\n");
    if (f_rename(argv[1], argv[2]) == -1)
        p_perror("fs_rename");
}

/**
 * cp SOURCE DEST
 * Copies SOURCE to DEST.
 */
void cpFunc(int argc, char **argv) {
    if (argc != 3)
        cmd_abort("Usage: cp SOURCE DEST\n");

    char buf[4096];
    ssize_t bytes_read;
    int src = f_open(argv[1], F_READ);
    if (src == -1) {
        p_perror("f_open");
        return;
    }
    int dest = f_open(argv[2], F_WRITE);
    if (dest == -1) {
        f_close(src);
        p_perror("f_open");
        return;
    }
    // read until there is nothing more
    do {
        // read a chunk
        bytes_read = f_read(src, 4096, buf);
        if (bytes_read == -1) {
            p_perror("f_read");
            break;
        }
        // then write that chunk to the dest
        if (f_write(dest, buf, bytes_read) != bytes_read) {
            p_perror("f_write");
            break;
        }
    } while (bytes_read);
    f_close(src);
    f_close(dest);
}

/**
 * rm FILE ...
 * Removes the files.
 */
void rmFunc(int argc, char **argv) {
    if (argc < 2)
        cmd_abort("Usage: rm FILE ...\n");
    // delete each file
    for (int i = 1; i < argc; ++i) {
        if (f_unlink(argv[i]) == -1)
            p_perror("f_unlink");
    }
}

/**
 * chmod ([+-=][rwx]*|OCT) FILE ...
 * Similar to chmod(1) in the VM.
 *
 * execute = 1
 * write = 2
 * read = 4
 */
void chmodFunc(int argc, char **argv) {
    if (argc < 3)
        cmd_abort("chmod ([+=-][rwx]*|OCT) FILE ...\n");
    if (fs == NULL)
        cmd_abort("You do not have a filesystem mounted\n");

    uint8_t bitset = 0;
    char mode;
    // parse arg 1: does it start with [+=-]?
    if (argv[1][0] == '+' || argv[1][0] == '=' || argv[1][0] == '-') {
        mode = argv[1][0];
        for (int i = 1; i < strlen(argv[1]); ++i) {
            if (argv[1][i] == 'x')
                bitset |= FAT_EXECUTE;
            else if (argv[1][i] == 'w')
                bitset |= FAT_WRITE;
            else if (argv[1][i] == 'r')
                bitset |= FAT_READ;
            else
                cmd_abort("mode must be of the form /[+=-][rwx]+/ or in the range [0..7]\n");
        }
    } else {
        // must be a single octal digit then
        if (!isdigit(argv[1][0]))
            cmd_abort("mode must be of the form /[+=-][rwx]+/ or in the range [0..7]\n");
        int arg = atoi(argv[1]);
        if (arg < 0 || arg > 7)
            cmd_abort("mode must be of the form /[+=-][rwx]+/ or in the range [0..7]\n");
        bitset = arg;
        mode = '=';
    }

    // open each file, then set its permission based on bitset and mode
    for (int i = 2; i < argc; ++i) {
        int fd = f_open(argv[i], F_APPEND);
        if (fd == -1) {
            p_perror("fs_open");
            return;
        }
        f_chmod(fd, mode, bitset);
        f_close(fd);
    }
}

void psFunc (int argc, char **argv){
    //loop through all queues 
    // print every process, it's own ucontext

    //jobs : in-shell process. talking about the current shell. bg/fg.. finished/stopped/ shows pipelines.. JOB_ID printed

    //logging : fputs, fprintf, fwrite, create a logging.c/.h file.//user programs should not directly write to log file.
    printf("PID PPID PRIORITY\n");
    // f_write(PSTDOUT_FILENO, "\n", sizeof("\n"));
    Process *temp = highQhead;
    while(temp != NULL){
        // printf("high\n");
        printf("%3d %4d %8d\n",temp->pcb->pid, temp->pcb->ppid, temp->pcb->priority);
        temp = temp->next;
    }
    temp = medQhead;
    while(temp != NULL){
        // printf("med\n");
        printf("%3d %4d %8d\n",temp->pcb->pid, temp->pcb->ppid, temp->pcb->priority);
        temp = temp->next;
    }
    temp = lowQhead;
    while(temp != NULL){
        // printf("low\n");
        printf("%3d %4d %8d\n",temp->pcb->pid, temp->pcb->ppid, temp->pcb->priority);
        temp = temp->next;
    }
    temp = blockedQhead;
    while(temp != NULL){
        // printf("block\n");
        printf("%3d %4d %8d\n",temp->pcb->pid, temp->pcb->ppid, temp->pcb->priority);
        temp = temp->next;
    }
    temp = stoppedQhead;
    while(temp != NULL){
        // printf("stop\n");
        printf("%3d %4d %8d\n",temp->pcb->pid, temp->pcb->ppid, temp->pcb->priority);
        temp = temp->next;
    }
    // temp = zombieQhead;
    // while(temp != NULL){
    //     printf("hi\n");
    //     printf("%3d %4d %8d\n",temp->pcb->pid, temp->pcb->ppid, temp->pcb->priority);
    //     temp = temp->next;
    // }
    // temp = orphanQhead;
    // while(temp != NULL){
    //     printf("hi\n");
    //     printf("%3d %4d %8d\n",temp->pcb->pid, temp->pcb->ppid, temp->pcb->priority);
    //     temp = temp->next;
    // }
}

void zombify(int argc, char **argv) {
    // p_spawn(zombie_child);
    pid_t pid = p_spawn(zombie_child, argv, PSTDIN_FILENO, PSTDOUT_FILENO);
    if (pid == -1){
        p_exit();
    }
    while (1) ;
    return;
}
void zombie_child() {
    printf("MMMMM Brains...!\n");
    return;
}

void orphan_child() {
    printf("Please sir, I want some more\n");
    while (1) ;
}
void orphanify(int argc, char **argv) {
    // p_spawn(orphan_child);
    pid_t pid = p_spawn(orphan_child, argv, PSTDIN_FILENO, PSTDOUT_FILENO);
    if (pid == -1){
        p_exit();
    }
    return;
}

void logout(){
    printf("Logging out\n");
    p_exit();
}