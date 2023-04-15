#include "user_functions.h"

void echoFunc(int argc, char *argv[]) {
    // printf("inside echoFunc() \n");
    for (int i = 1; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    
}

void sleepFunc(char *argv[]) {
    // int OGticksLeft = ticksLeft;
    dequeue(activeProcess);
    int ticksLeft = 10*atoi(argv[1]);
    activeProcess->pcb->sleep_time_remaining = ticksLeft;
    enqueueBlocked(activeProcess);
    
    swapcontext(activeContext, &schedulerContext);
    
}

void busy_wait(void) {
    while (1) {
        // Do nothing and keep looping indefinitely
    }
}


// void ls() {
//     DIR* directory = opendir(".");
//     if (directory == NULL) {
//         printf("Error: Unable to open directory\n");
//         exit(1);
//     }

//     struct dirent* file;
//     struct stat file_stat;
//     struct passwd* user_info;
//     struct group* group_info;

//     printf("%-10s %-8s %-8s %-8s %-10s %-12s %-12s %s\n", "Permissions", "Links", "User", "Group", "Size", "Modified", "Inode", "Filename");

//     while ((file = readdir(directory)) != NULL) {
//         char* filename = file->d_name;

//         if (stat(filename, &file_stat) < 0) {
//             printf("Error: Unable to get file status for %s\n", filename);
//             continue;
//         }

//         char permissions[11];
//         permissions[0] = (S_ISDIR(file_stat.st_mode)) ? 'd' : '-';
//         permissions[1] = (file_stat.st_mode & S_IRUSR) ? 'r' : '-';
//         permissions[2] = (file_stat.st_mode & S_IWUSR) ? 'w' : '-';
//         permissions[3] = (file_stat.st_mode & S_IXUSR) ? 'x' : '-';
//         permissions[4] = (file_stat.st_mode & S_IRGRP) ? 'r' : '-';
//         permissions[5] = (file_stat.st_mode & S_IWGRP) ? 'w' : '-';
//         permissions[6] = (file_stat.st_mode & S_IXGRP) ? 'x' : '-';
//         permissions[7] = (file_stat.st_mode & S_IROTH) ? 'r' : '-';
//         permissions[8] = (file_stat.st_mode & S_IWOTH) ? 'w' : '-';
//         permissions[9] = (file_stat.st_mode & S_IXOTH) ? 'x' : '-';
//         permissions[10] = '\0';

//         user_info = getpwuid(file_stat.st_uid);
//         group_info = getgrgid(file_stat.st_gid);

//         printf("%-10s %-8ld %-8s %-8s %-10ld %-12s %-12lu %s\n", permissions, (long)file_stat.st_nlink, user_info->pw_name, group_info->gr_name, (long)file_stat.st_size, ctime(&file_stat.st_mtime), (long)file_stat.st_ino, filename);
//     }

//     closedir(directory);
// }

// void touchFunc(const char* filename) {
//     int fd = open(filename, O_CREAT, S_IRUSR | S_IWUSR);
//     if (fd < 0) {
//         printf("Error: Unable to create or open file\n");
//         exit(1);
//     }
//     close(fd);
//     utime(filename, NULL);
// }
