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
    f_write(PSTDOUT_FILENO, "\n", 1);
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
        fprintf(fp, "%d\t%c%c%c\t%d\t%s\t%s\n", entry->blockno, x, r, w, entry->size, date, entry->name);
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

char psStatus(int stat){
    char procStatus = '\0';

    switch (stat){
    case BLOCKED:
        procStatus = 'B';
        break;

    case RUNNING:
        procStatus = 'R';
        break;

    case STOPPED:
        procStatus = 'S';
        break;

    case ZOMBIE:
        procStatus = 'Z';
        break;
    
    default:
        break;
    }

    return procStatus;
}

void psFunc (int argc, char **argv){
    //loop through all queues 
    // print every process, it's own ucontext

    //jobs : in-shell process. talking about the current shell. bg/fg.. finished/stopped/ shows pipelines.. JOB_ID printed

    //logging : fputs, fprintf, fwrite, create a logging.c/.h file.//user programs should not directly write to log file.

    char stat = '\0';

    char argbuf[strlen("PID PPID PRIORITY STAT\n") + 1];
    sprintf(argbuf, "%s ","PID PPID PRIORITY STAT\n");
    f_write(PSTDOUT_FILENO, argbuf, strlen("PID PPID PRIORITY STAT\n") + 1);
    // printf("PID PPID PRIORITY\n");
    Process *temp = highQhead;
    while(temp != NULL){
        // printf("high\n");
        // f_write(PSTDOUT_FILENO, "\n", sizeof("\n"));
        stat = psStatus(temp->pcb->status);
        char argbuf[strlen("PID PPID PRIORITY STAT\n") + 1];
        sprintf(argbuf, "%d %4d %5d %8c\n",temp->pcb->pid, temp->pcb->ppid, temp->pcb->priority, stat);
        f_write(PSTDOUT_FILENO, argbuf, strlen("PID PPID PRIORITY STAT\n") + 1);
        // printf("%3d %4d %8d\n",temp->pcb->pid, temp->pcb->ppid, temp->pcb->priority);
        temp = temp->next;
    }
    temp = medQhead;
    while(temp != NULL){
        // printf("med\n");
        stat = psStatus(temp->pcb->status);
        char argbuf[strlen("PID PPID PRIORITY STAT\n") + 1];
        sprintf(argbuf, "%d %3d %5d %8c\n",temp->pcb->pid, temp->pcb->ppid, temp->pcb->priority, stat);
        f_write(PSTDOUT_FILENO, argbuf, strlen("PID PPID PRIORITY STAT\n") + 1);
        // printf("%d %3d %4d\n",temp->pcb->pid, temp->pcb->ppid, temp->pcb->priority);
        temp = temp->next;
    }
    temp = lowQhead;
    while(temp != NULL){
        // printf("low\n");
        // printf("%d %3d %4d\n",temp->pcb->pid, temp->pcb->ppid, temp->pcb->priority);
        stat = psStatus(temp->pcb->status);
        char argbuf[strlen("PID PPID PRIORITY STAT\n") + 1];
        sprintf(argbuf, "%d %3d %4d %8c\n",temp->pcb->pid, temp->pcb->ppid, temp->pcb->priority, stat);
        f_write(PSTDOUT_FILENO, argbuf, strlen("PID PPID PRIORITY STAT\n") + 1);
        temp = temp->next;
    }
    temp = blockedQhead;
    while(temp != NULL){
        // printf("block\n");
        // printf("%d %3d %4d\n",temp->pcb->pid, temp->pcb->ppid, temp->pcb->priority);
        stat = psStatus(temp->pcb->status);
        char argbuf[strlen("PID PPID PRIORITY STAT\n") + 1];
        sprintf(argbuf, "%d %3d %4d %8c\n",temp->pcb->pid, temp->pcb->ppid, temp->pcb->priority, stat);
        f_write(PSTDOUT_FILENO, argbuf, strlen("PID PPID PRIORITY STAT\n") + 1);
        temp = temp->next;
    }
    temp = stoppedQhead;
    while(temp != NULL){
        // printf("stop\n");
        // printf("%d %3d %4d\n",temp->pcb->pid, temp->pcb->ppid, temp->pcb->priority);
        stat = psStatus(temp->pcb->status);
        char argbuf[strlen("PID PPID PRIORITY STAT\n") + 1];
        sprintf(argbuf, "%d %3d %4d %8c\n",temp->pcb->pid, temp->pcb->ppid, temp->pcb->priority, stat);
        f_write(PSTDOUT_FILENO, argbuf, strlen("PID PPID PRIORITY STAT\n") + 1);
        temp = temp->next;
    }
    temp = zombieQhead;
    while(temp != NULL){
        // printf("hi\n");
        // printf("%d %3d %4d\n",temp->pcb->pid, temp->pcb->ppid, temp->pcb->priority);
        stat = psStatus(temp->pcb->status);
        char argbuf[strlen("PID PPID PRIORITY STAT\n") + 1];
        sprintf(argbuf, "%d %3d %4d %8c\n",temp->pcb->pid, temp->pcb->ppid, temp->pcb->priority, stat);
        f_write(PSTDOUT_FILENO, argbuf, strlen("PID PPID PRIORITY STAT\n") + 1);
        temp = temp->next;
    }
    temp = orphanQhead;
    while(temp != NULL){
        printf("hi\n");
        printf("%d %3d %4d %8c\n",temp->pcb->pid, temp->pcb->ppid, temp->pcb->priority, stat);
        temp = temp->next;
    }
}

void zombify(int argc, char **argv) {

    pid_t pid = p_spawn(zombie_child, argv, PSTDIN_FILENO, PSTDOUT_FILENO);
    if (pid == -1){
        p_exit();
    }
    while (1) ;
    return;
}
void zombie_child() {
    char argbuf[4096];
    sprintf(argbuf, "MMMMM Brains...!\n"); 
    f_write(PSTDOUT_FILENO, argbuf, 4096);
    return;
}

void orphanify(int argc, char **argv) {
    // p_spawn(orphan_child);
    pid_t pid = p_spawn(orphan_child, argv, PSTDIN_FILENO, PSTDOUT_FILENO);
    if (pid == -1){
        p_exit();
    }
}

void orphan_child() {
    char argbuf[4096];
    sprintf(argbuf, "Please sir, I want some more\n"); 
    f_write(PSTDOUT_FILENO, argbuf, 4096);
    while (1) ;
}

void logout(){
    char argbuf[4096];
    argbuf[0]='\0';
    sprintf(argbuf, "Logging out\n"); 
    f_write(PSTDOUT_FILENO, argbuf, strlen(argbuf)+1);
    // printf("Logging out\n");
    p_exit();
}

void killFunc(int argc, char **argv){
    char *s = "term";
    int pid;
    if (argc == 3){
        s = argv[1];
        pid = atoi(argv[2]);
    }
    else if (argc == 2){
        pid = atoi(argv[1]);
    }
    else{
        p_perror("invalid command");
        return;
    }
    
    // printf("signal %s argc %d", signal, argc);
    if (strcmp(s,"stop")==0){
        if (p_kill(pid,S_SIGSTOP) < 0){
            p_perror("error in killing process");
        }
    } else if (strcmp(s,"cont")==0){
        if (p_kill(pid,S_SIGCONT) < 0){
            p_perror("error in killing process");
        }
    } else if (strcmp(s,"term")==0){
        // p_kill(pid,S_SIGTERM);
        if (p_kill(pid,S_SIGTERM) < 0){
            p_perror("error in killing process");
        }
    }
    
    return;
}

void man(){

    char argbuf[200];
    // char s[200];
    // sprintf(argbuf, "%s",s);
    
    char s[] = "mkfs FS_NAME BLOCKS_IN_FAT BLOCK_SIZE_CONFIG\t - Creates a PennFAT filesystem in the file named FS_NAME\n";
    sprintf(argbuf,"%s",s);  
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s));
    char s1[] = "mount FS_NAME\t - Mounts the filesystem named FS_NAME by loading its FAT into memory\n";
    sprintf(argbuf,"%s",s1); 
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s1));
    char s2[] = "umount\t - Unmounts the currently mounted filesystem\n";
    sprintf(argbuf,"%s",s2); 
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s2));
    char s3[] = "touch FILE\t - Creates the files if they do not exist, or updates their timestamp to the current system time";
    sprintf(argbuf,"%s",s3); 
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s3));
    char s4[] =  "mv SOURCE DEST\t - Renames SOURCE to DEST\n";
    sprintf(argbuf,"%s",s4);
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s4));
    char s5[] =  "rm FILE\t - Removes the files\n";
    sprintf(argbuf,"%s",s5);
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s5));
    char s6[] = "cat FILE ... [ -w OUTPUT_FILE ]\t - Concatenates the files and prints them to stdout by default, or overwrites OUTPUT_FILE. If OUTPUT_FILE does not exist, it will be created.\n";
    sprintf(argbuf,"%s",s6); 
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s6));
    char s7[] = "cat FILE ... [ -a OUTPUT_FILE ]\t - Concatenates the files and prints them to stdout by default, or appends to OUTPUT_FILE.\n";
    sprintf(argbuf,"%s",s7); 
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s7));
    char s8[] = "cat -w OUTPUT_FILE\t - Reads from the terminal and overwrites OUTPUT_FILE.\n";
    sprintf(argbuf,"%s",s8); 
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s8));
    char s9[] = "cat -a OUTPUT_FILE\t - Reads from the terminal and appends to OUTPUT_FILE.\n";
    sprintf(argbuf,"%s",s9); 
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s9));
    char s10[]="cp [ -h ] SOURCE DEST\t - Copies SOURCE to DEST. With -h, SOURCE is from the host OS\n";
    sprintf(argbuf,"%s",s10); 
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s10));
    char s11[]="cp SOURCE -h DEST\t - Copies SOURCE from the filesystem to DEST in the host OS.\n";
    sprintf(argbuf,"%s",s11); 
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s11));
    char s12[]="ls\t - List all files in the directory\n";
    sprintf(argbuf,"%s",s12); 
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s12));
    char s13[]="chmod ([+=-][rwx]*|OCT) FILE \t - To change permissions of a file\n";
    sprintf(argbuf,"%s",s13); 
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s13));
    char s14[]="sleep n\t - sleep for n seconds\n";
    sprintf(argbuf,"%s",s14); 
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s14));
    char s15[]="busy\t - usy wait indefinitely\n";
    sprintf(argbuf,"%s",s15); 
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s15));
    char s16[]="echo\t - Write arguments to the standard output\n";
    sprintf(argbuf,"%s",s16); 
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s16));
    char s17[]="ps\t - list all processes on PennOS. Displays pid, ppid, and priority\n";
    sprintf(argbuf,"%s",s17); 
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s17));
    char s18[]="kill [ -SIGNAL_NAME ] pid\t - end the specified signal to the specified processes, where -SIGNAL_NAME is either term (the default), stop, or cont\n";
    sprintf(argbuf,"%s",s18); 
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s18));
    char s19[]="zombify\t - To check if PennOS can handle zombies\n";
    sprintf(argbuf,"%s",s19); 
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s19));
    char s20[]="orphanify\t - To check if PennOS can handle orphans\n";
    sprintf(argbuf,"%s",s20); 
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s20));
    char s21[]="nice priority command [arg]\t - set the priority of the command to priority and execute the command\n";
    sprintf(argbuf,"%s",s21); 
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s21));
    char s22[]="nice_pid priority pid\t - adjust the nice level of process pid to priority priority.\n";
    sprintf(argbuf,"%s",s22); 
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s22));
    char s23[]="bg [job_id]\t - continue the last stopped job, or the job specified by job_id\n";
    sprintf(argbuf,"%s",s23); 
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s23));
    char s24[]="fg [job_id]\t - bring the last stopped or backgrounded job to the foreground, or the job specified by job_id\n";
    sprintf(argbuf,"%s", s24);
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s24));
    char s25[]="jobs\t - list all jobs\n";
    sprintf(argbuf,"%s", s25);
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s25));
    char s26[]="logout\t - exit the shell and shutdown PennOS\n";
    sprintf(argbuf,"%s", s26);
    f_write(PSTDOUT_FILENO, argbuf, sizeof(s26));
    return;
}

