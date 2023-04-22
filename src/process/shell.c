#include "shell.h"

char **bufferSig;
int async = 0;
int IS_BG = 0;
int pgid = 0;
pid_t curr_pid = 0;
int par_pid = 0;
int bufferWaiting = 0;
int bufferCount = 0;
int fgpid = 1;
pid_t newpid = 0;
int flag_spaces = 0; 
int retFlag = 0;

char* strCopy(char* src, char* dest) {
    strtok(src, "&");
    int i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

struct Job *createJob(int pid, int bgFlag, int numChildren, char *input){
    struct Job *newJob;
    newJob = (struct Job *)malloc(sizeof(struct Job));
    newJob -> commandInput = malloc((strlen(input) + 1) * sizeof(char));
    strcpy(newJob -> commandInput, input);
    newJob -> next = NULL;
    newJob -> bgFlag = bgFlag;
    newJob -> myPid = pid;
    newJob -> status = RUNNING;
    return newJob;
}

void setTimer(void) {
    struct itimerval it;

    it.it_interval = (struct timeval){.tv_usec = quantum};
    it.it_value = it.it_interval;

    setitimer(ITIMER_REAL, &it, NULL);
}

void sigIntTermHandler(int signal) {
    // ignore for bg processes
    if(signal == SIGINT){
        if(fgpid > 1){
            p_kill(fgpid, S_SIGTERM);
        }
        if(fgpid == 1){
            f_write(PSTDOUT_FILENO, "\n", sizeof("\n"));
            f_write(PSTDOUT_FILENO, PROMPT, sizeof(PROMPT));
        }
    }
}

void sigcontHandler(int signal){
    if(signal == SIGTSTP){
        if(fgpid > 1){
            p_kill(fgpid, S_SIGCONT);
        }
        if(fgpid == 1){
            p_kill(fgpid, S_SIGCONT);
            // f_write(PSTDOUT_FILENO, "\n", sizeof("\n"));
            // f_write(PSTDOUT_FILENO, PROMPT, sizeof(PROMPT));
        }
    }
}

void sigtstpHandler(int signal){
    if(signal == SIGTSTP){ //ctrl-z
        if(fgpid > 1){ // if not shell running in fg
            int ret = p_kill(fgpid, S_SIGSTOP);  // stop that process
            // printf("exit only becasue nothing to run, need to exit using p_exit");
            if (ret == -1){
                p_exit();
            }
        }
        if(fgpid == 1){ // if shell is running, do nothing
            f_write(PSTDOUT_FILENO, "\n", sizeof("\n"));
            f_write(PSTDOUT_FILENO, PROMPT, sizeof(PROMPT));
        }
    }
}

void setSignalHandler(void){

    struct sigaction sa_alarm;

    sa_alarm.sa_handler = alarmHandler;
    sa_alarm.sa_flags = SA_RESTART;
    sigfillset(&sa_alarm.sa_mask);

    sigaction(SIGALRM, &sa_alarm, NULL);

    struct sigaction sa_int;

    sa_int.sa_handler = sigIntTermHandler;
    sa_int.sa_flags = SA_RESTART;
    sigfillset(&sa_int.sa_mask);

    sigaction(SIGINT, &sa_int, NULL);

    struct sigaction sa_term;

    sa_term.sa_handler = sigIntTermHandler;
    sa_term.sa_flags = SA_RESTART;
    sigfillset(&sa_term.sa_mask);

    sigaction(SIGTERM, &sa_term, NULL);

    struct sigaction sa_cont;

    sa_cont.sa_handler = sigcontHandler;
    sa_cont.sa_flags = SA_RESTART;
    sigfillset(&sa_cont.sa_mask);

    sigaction(SIGCONT, &sa_cont, NULL);

    struct sigaction sa_stop;

    sa_stop.sa_handler = sigtstpHandler;
    sa_stop.sa_flags = SA_RESTART;
    sigfillset(&sa_cont.sa_mask);

    sigaction(SIGTSTP, &sa_stop, NULL);
}

// clear all the mallocs to prevent memory leaks
void freeOneJob(struct Job *Job){
    free(Job -> commandInput);
    free(Job);
}

// call freeOneJob for every job in order to clear the entire LL memory
void freeAllJobs(struct Job *head) {
    // if the head is null, nothing to clear
    if (head == NULL) {
        return;
    }

    // iterate through LL, call freeOneJob
    struct Job * current = head;
        while (current != NULL) {
            struct Job* removal = current;
            current = current -> next;
            freeOneJob(removal);
    }
}

// input parameters: head of LL, newJob we want to add to LL
struct Job *addJob(struct Job *head, struct Job *newJob){

    // if there are no jobs in the LL yet, create one, assign #1
    if (head == NULL){ 
        head = newJob; 
        newJob -> JobNumber = 1;
        // printf("adding to shell queue:  %s\n", newJob->commandInput);
        // printf("Shell Q after adding head: \n");
        // iterateShell(head);
        return head;
    }
    
    // If there is only one job currently, this will be job #2
    if (head -> next == NULL){
        head -> next = newJob;
        newJob -> JobNumber = 2;
        // printf("adding to shell queue:  %s\n", newJob->commandInput);
        // printf("Shell Q after adding head.next: \n");
        // iterateShell(head);
        return head;
    }

    // if job #1 has been removed, the head will point to a job with a number greater than 1. So add the new job as job #1. 
    if (head -> JobNumber > 1) { 
        newJob -> JobNumber = 1;
        newJob -> next = head;
        return newJob;
    }

    struct Job *current = head -> next;

    // check if the difference in job numbers through the LL continues to be 1
    while (current -> next != NULL && current -> next -> JobNumber - current -> JobNumber == 1){
        current = current -> next;
    }

    // Adding the Job to the end of the linked list since the last job points to null
    if (current -> next == NULL){
        newJob -> JobNumber = current -> JobNumber + 1;
        current -> next = newJob;
        newJob -> next = NULL;
        // printf("Shell Q after adding to end of LL: \n");
        // iterateShell(head);
        return head;
    }

    // if there is a gap in job numbers, fill in that gap and link both ends of the new job
    if (current -> next -> JobNumber - current -> JobNumber > 1){
        newJob -> JobNumber = current -> JobNumber + 1;
        current -> next = newJob;
        newJob -> next = current -> next;
        // printf("Shell Q after adding to gap: \n");
        // iterateShell(head);
        return head;
    }
    return head;
}

// Removes a job give a specifc job number.
struct Job *removeJob(struct Job *head, int jobNum){
    if (head == NULL) {
        // If the list is empty, return NULL
        return NULL;
    }

    if (head->JobNumber == jobNum) {
        struct Job *newHead = head->next;
        return newHead;
    }

    struct Job *current = head;
    while (current->next != NULL) {
        if (current->next->JobNumber == jobNum) {
            current->next = current->next->next;
            break;
        }
        current = current->next;
    }

    // If the job number was not found in the list, return the original head
    return head;

}

struct Job *getJob(struct Job *head, int jobNum){
    if (jobNum == 1){
        return head;
    }
    // iterate through all jobs until job of interest is reached
    struct Job *current = head;
    while (current -> next != NULL){
        current = current -> next;
        // if the next job is the one, replace next with the one after that
        if (current -> JobNumber == jobNum){
            return current;
        }
    }
    // freeOneJob(current); 
    char argbuf[strlen("No job with this ID found\n") + 1];
    sprintf(argbuf, "%s", "No job with this ID found\n");
    f_write(PSTDOUT_FILENO, argbuf, strlen(argbuf + 1));
    // exit(EXIT_FAILURE);
    p_exit();
    return current;
}

int getCurrentJob(struct Job *head){
    if(head -> next == NULL){
        return head -> JobNumber;
    }
    int bgNum = 0;
    int stpNum = 0;
    // iterate through all jobs until job of interest is reached
    struct Job *current = head;
    do{
        // if the next job is the one, replace next with the one after that
        if(current -> status == STOPPED){
            stpNum = current -> JobNumber;
        }
        else if(current -> bgFlag == BG){
            bgNum = current -> JobNumber;
        }
        current = current -> next;
    } while (current != NULL);
    
    if(stpNum != 0){
        return stpNum;
    }
    else if(bgNum != 0){
        return bgNum;
    }
    else{
        fprintf(stderr,"No bg or stopped jobs found\n");
        // exit(EXIT_FAILURE);
        p_exit();
        return -1;
    }
}

void changeStatus(struct Job *head, int jobNum, int newStatus){
    if (jobNum == 1){
        if(newStatus == 2){
            head -> status = RUNNING;
        }
        else if (newStatus == 3){
            head -> status = STOPPED;
        }
        else if(newStatus == 0){
            head->status = TERMINATED;
        }
        return;
    }
    // iterate through all jobs until job of interest is reached
    struct Job *current = head;
    while (current -> next != NULL){
        current = current -> next;
        // if the next job is the one, replace next with the one after that
        if (current -> JobNumber == jobNum){
            if(newStatus == 2){
                current -> status = RUNNING;
            }
            else if (newStatus == 3){
                current -> status = STOPPED;
            }
            else if (newStatus == 0){
                current->status = TERMINATED;
            }
        }
    }
}


void changeFGBG(struct Job *head, int jobNum, int newFGBG){
    if (jobNum == 1){
        if(newFGBG == 0){
            head -> bgFlag = FG;
        }
        else{
            head -> bgFlag = BG;
        } 
    }
    // iterate through all jobs until job of interest is reached
    struct Job *current = head;
    while (current -> next != NULL){
        current = current -> next;
        if (current -> JobNumber == jobNum){
            if(newFGBG == 0){
                current -> bgFlag = FG;
            }
            else{
                current -> bgFlag = BG;
            }
        }
    }
}

char *statusToStr(int status){
    if(status == 2){
        return "running";
    }
    else if(status == 3){
        return "stopped";
    }
    else if(status == 0){
        return "finished";
    }
    else{
        return "ready";
    }
}

// void iterateShell(struct Job *head){
//     fprintf(stderr,"Shell Queue Contains: \n");
//     if(head == NULL){
//         fprintf(stderr,"Shell Q Empty\n");
//         return;
//     }
//     while(head!= NULL){
//         fprintf(stderr,"%s\n", head->commandInput);
//         head = head->next;
//     }
// }

struct Job *head = NULL;

bool W_WIFEXITED(int status) {
    return status == TERMINATED;
}

bool W_WIFSTOPPED(int status) {
    return status == STOPPED;
}

bool W_WIFSIGNALED(int status) {
    return status == SIG_TERMINATED;
}

void shellBFunc(struct parsed_command *cmd){
    int i = 0;
    if(strcmp(cmd->commands[0][0], "nice") == 0){
        i = 2;
    }
    
    // spawn builtin
    if (strcmp(cmd->commands[0][i], "sleep") == 0) {
        curr_pid = p_spawn(sleepFunc, cmd->commands[0], PSTDIN_FILENO, PSTDOUT_FILENO);
    } else if (strcmp(cmd->commands[0][i], "echo") == 0) {
        curr_pid = p_spawn(echoFunc, cmd->commands[0], PSTDIN_FILENO, PSTDOUT_FILENO);
    } else if (strcmp(cmd->commands[0][i], "busy") == 0) {
        curr_pid = p_spawn(busyFunc, cmd->commands[0], PSTDIN_FILENO, PSTDOUT_FILENO);
    } else if (strcmp(cmd->commands[0][i], "ps") == 0) {
        curr_pid = p_spawn(psFunc, cmd->commands[0], PSTDIN_FILENO, PSTDOUT_FILENO);
    } else if (strcmp(cmd->commands[0][i], "kill") == 0) {
        curr_pid = p_spawn(killFunc, cmd->commands[0], PSTDIN_FILENO, PSTDOUT_FILENO);
    } else if (strcmp(cmd->commands[0][i], "zombify") == 0) {
        curr_pid = p_spawn(zombify, cmd->commands[0], PSTDIN_FILENO, PSTDOUT_FILENO);
    } else if (strcmp(cmd->commands[0][i], "orphanify") == 0) {
        curr_pid = p_spawn(orphanify, cmd->commands[0], PSTDIN_FILENO, PSTDOUT_FILENO);
    } else if (strcmp(cmd->commands[0][i], "logout") == 0) {
        logout();
        retFlag = 1;
        return;
    } else if (strcmp(cmd->commands[0][i], "man") == 0) {
        man();
        retFlag = 1;
        return;
    }
    else if (strcmp(cmd->commands[0][i], "cat") == 0) {
        curr_pid = p_spawn(catFunc, cmd->commands[0], PSTDIN_FILENO, PSTDOUT_FILENO);
    } else if (strcmp(cmd->commands[0][i], "ls") == 0) {
        curr_pid = p_spawn(lsFunc, cmd->commands[0], PSTDIN_FILENO, PSTDOUT_FILENO);
    } else if (strcmp(cmd->commands[0][i], "touch") == 0) {
        curr_pid = p_spawn(touchFunc, cmd->commands[0], PSTDIN_FILENO, PSTDOUT_FILENO);
    } else if (strcmp(cmd->commands[0][i], "mv") == 0) {
        curr_pid = p_spawn(mvFunc, cmd->commands[0], PSTDIN_FILENO, PSTDOUT_FILENO);
    } else if (strcmp(cmd->commands[0][i], "cp") == 0) {
        curr_pid = p_spawn(cpFunc, cmd->commands[0], PSTDIN_FILENO, PSTDOUT_FILENO);
    } else if (strcmp(cmd->commands[0][i], "rm") == 0) {
        curr_pid = p_spawn(rmFunc, cmd->commands[0], PSTDIN_FILENO, PSTDOUT_FILENO);
    } else if (strcmp(cmd->commands[0][i], "chmod") == 0) {
        curr_pid = p_spawn(chmodFunc, cmd->commands[0], PSTDIN_FILENO, PSTDOUT_FILENO);
    }
}

void pennShredder(char* buffer){
    IS_BG = 0;
    int numBytes = strlen(buffer);

    // exit iteration if only new line given
    if (numBytes == 1 && buffer[0] == '\n') {
        return;
    }

    buffer[numBytes] = '\0'; // set last char of buffer to null to prevent memory leaks
    
    struct parsed_command *cmd;
    int num = parse_command(buffer, &cmd);

    if(num != 0){
        return;
    }
    
    // check for BG builtin
    if(strcmp("bg", cmd -> commands[0][0]) == 0){
        if(head == NULL){
            char argbuf[strlen("No jobs present in the queue \n") + 1];
            sprintf(argbuf, "%s ", "No jobs present in the queue \n");
            f_write(PSTDOUT_FILENO, argbuf, strlen(argbuf) + 1);
            // fprintf(stderr, "No jobs present in the queue \n");
            free(cmd);
            return;
        }
        
        // CASE WHERE JOB ID IS GIVEN
        if(cmd -> commands[0][1] != NULL){
            int job_id = atoi(cmd -> commands[0][1]);
            
            struct Job *bgJob = getJob(head, job_id);

            if (bgJob -> status == STOPPED){
                // Send a SIGCONT signal to the process to continue it in the background
                changeStatus(head, job_id, 2); // set job to running
                changeFGBG(head, job_id, 1); // set job to BG 
                char argbuf[1024];
                argbuf[0]='\0';
                sprintf(argbuf, "Running: %s ", bgJob -> commandInput);
                f_write(PSTDOUT_FILENO, argbuf, strlen(argbuf) + 1);
                // fprintf(stderr,"Running: %s", bgJob -> commandInput);
                p_kill(bgJob -> myPid, S_SIGCONT); // killpg(bgJob -> pgid, SIGCONT);
                free(cmd);
                return;
            } 
            // if running, move from bg to fg
            else if (bgJob -> status == RUNNING){
                char argbuf[1024];
                argbuf[0]='\0';
                sprintf(argbuf, "%s already running\n", bgJob -> commandInput);
                f_write(PSTDOUT_FILENO, argbuf, strlen(argbuf) + 1);
                // fprintf(stderr,"%s already running\n", bgJob -> commandInput);
                // changeFGBG(head, job_id, 1); // set job to BG 
                // fprintf(stderr,"Running: %s", bgJob -> commandInput);
                free(cmd);
                return;
            }  
        }
        else{
            // case where no job ID given
            int job_id = getCurrentJob(head);
            struct Job *bgJob = getJob(head, job_id);
            if (bgJob -> status == STOPPED){
                // Send a SIGCONT signal to the process to continue it in the background
                changeStatus(head, job_id, 2); // set job to running
                changeFGBG(head, job_id, 1); // set job to BG 
                char argbuf[1024];
                argbuf[0]='\0';
                sprintf(argbuf, "Running: %s", bgJob -> commandInput);
                f_write(PSTDOUT_FILENO, argbuf, strlen(argbuf) + 1);
                // fprintf(stderr,"Running: %s", bgJob -> commandInput);
                // printf("TEST\n");
                p_kill(bgJob -> myPid, S_SIGCONT); // killpg(bgJob -> pgid, SIGCONT);
                free(cmd);
                return;
            }
            else if(bgJob->status == RUNNING){
                char argbuf[strlen(bgJob -> commandInput) + 1];
                sprintf(argbuf, "%s already running\n", bgJob -> commandInput);
                f_write(PSTDOUT_FILENO, argbuf, strlen(bgJob -> commandInput) + 1);
                // fprintf(stderr,"%s already running\n", bgJob -> commandInput);
                // changeFGBG(head, job_id, 1); // set job to BG 
                // fprintf(stderr,"Running: %s", bgJob -> commandInput);
                free(cmd);
                return;
            }
            free(cmd);
            return;
        }
    }

   
    // check for FG builtin
    if(strcmp("fg", cmd -> commands[0][0]) == 0){
        if(head == NULL){
            // fprintf(stderr, "No jobs present in the queue \n");
            char argbuf[strlen("No jobs present in the queue \n")+1];
            sprintf(argbuf, "No jobs present in the queue \n"); 
            f_write(PSTDOUT_FILENO, argbuf, strlen("No jobs present in the queue \n") + 1);
            free(cmd);
            return;
        }
        
        // case where JID is given
        if(cmd -> commands[0][1] != NULL){
            int job_id = atoi(cmd -> commands[0][1]);
            struct Job *fgJob = getJob(head, job_id);
            int pid_fg = fgJob->myPid;
            if (fgJob -> status == STOPPED){
                // Send a SIGCONT signal to the process to continue it in the background
                changeStatus(head, job_id, 2); // set job to running
                changeFGBG(head, job_id, 0); // set job to FG 
                fgpid = pid_fg;
                // fprintf(stderr, "Restarting: %s", fgJob -> commandInput);
                char argbuf[strlen(fgJob->commandInput)+1];
                sprintf(argbuf, "Restarting: %s\n", fgJob->commandInput); 
                f_write(PSTDOUT_FILENO, argbuf, strlen(fgJob->commandInput) + 1);

                p_kill(pid_fg, S_SIGCONT);   
                head = removeJob(head, fgJob->JobNumber);

                int status; 
                p_waitpid(pid_fg, &status, FALSE);
                
                if(W_WIFSTOPPED(status)){ 
                    // fprintf(stderr, "Stopped: %s\n", fgJob -> commandInput); 
                    char argbuf[strlen(fgJob->commandInput)+1];
                    sprintf(argbuf, "Stopped: %s\n", fgJob->commandInput); 
                    f_write(PSTDOUT_FILENO, argbuf, strlen(fgJob->commandInput) + 1);
                    fgJob -> status = STOPPED; 
                    // if (bufferWaiting){
                    //     //PRINT BUFFER
                    //     for (int i = 0; i < bufferCount ; i++) {
                    //         fprintf(stderr, "%s\n", bufferSig[i]);
                    //     }
                    //     free(bufferSig);
                    //     bufferWaiting = 0;
                    //     bufferCount = 0;
                    // }
                } 
                fgpid = 1;
                free(cmd);
                return;
            }
            // not stopped, but running in BG
            else{
                changeFGBG(head, job_id, 0); // set job to FG 
                fprintf(stderr,"%s\n", fgJob -> commandInput); 
                fgpid = pid_fg;
                p_kill(pid_fg, S_SIGCONT); 
                head = removeJob(head, fgJob->JobNumber);
     
                int status;
                p_waitpid(pid_fg, &status, FALSE);

                if(W_WIFSTOPPED(status)){ 
                    // fprintf(stderr, "Stopped: %s\n", fgJob -> commandInput); 
                    char argbuf[50];
                    sprintf(argbuf, "Stopped: %s\n", fgJob->commandInput); 
                    f_write(PSTDOUT_FILENO, argbuf, strlen(fgJob->commandInput) + 1);
                    fgJob -> status = STOPPED; 
                    // if (bufferWaiting){
                    //     //PRINT BUFFER
                    //     for (int i = 0; i < bufferCount ; i++) {
                    //         fprintf(stderr, "%s\n", bufferSig[i]);
                    //     }
                    //     free(bufferSig);
                    //     bufferWaiting=0;
                    //     bufferCount = 0;
                    // }
                } 
                fgpid = 1;
                free(cmd);
                return;
            }
        }
        else{ // case where no job ID given
            int job_id = getCurrentJob(head);
            struct Job *fgJob = getJob(head, job_id);
            int pid_fg = fgJob->myPid;
            if (fgJob -> status == STOPPED){
                // Send a SIGCONT signal to the process to continue it in the background
                changeStatus(head, job_id, 2); // set job to running
                changeFGBG(head, job_id, 0); // set job to FG 
                fgpid = pid_fg;
                fprintf(stderr, "Restarting: %s", fgJob -> commandInput);
                p_kill(pid_fg, S_SIGCONT);   
                head = removeJob(head, fgJob->JobNumber);

                int status; 
                p_waitpid(pid_fg, &status, FALSE);
                
                if(W_WIFSTOPPED(status)){ 
                    // fprintf(stderr, "Stopped: %s\n", fgJob -> commandInput); 

                    char argbuf[50];
                    sprintf(argbuf, "Stopped: %s\n", fgJob->commandInput); 
                    f_write(PSTDOUT_FILENO, argbuf, strlen(fgJob->commandInput) + 1);

                    fgJob -> status = STOPPED; 
                    // if (bufferWaiting){
                    //     //PRINT BUFFER
                    //     for (int i = 0; i < bufferCount ; i++) {
                    //         fprintf(stderr, "%s\n", bufferSig[i]);
                    //     }
                    //     free(bufferSig);
                    //     bufferWaiting = 0;
                    //     bufferCount = 0;
                    // }
                } 
                fgpid = 1;
                free(cmd);
                return;
            }
            // not stopped, but running in BG
            else{
                changeFGBG(head, job_id, 0); // set job to FG 
                fprintf(stderr,"%s\n", fgJob -> commandInput); 
                fgpid = pid_fg;
                p_kill(pid_fg, S_SIGCONT); 
                head = removeJob(head, fgJob->JobNumber);
     
                int status;
                p_waitpid(pid_fg, &status, FALSE);

                if(W_WIFSTOPPED(status)){ 
                    
                    // fprintf(stderr, "Stopped: %s\n", fgJob -> commandInput); 
                    char argbuf[50];
                    sprintf(argbuf, "Stopped: %s\n", fgJob->commandInput); 
                    f_write(PSTDOUT_FILENO, argbuf, strlen(fgJob->commandInput) + 1);


                    fgJob -> status = STOPPED; 
                    // if (bufferWaiting){
                    //     //PRINT BUFFER
                    //     for (int i = 0; i < bufferCount ; i++) {
                    //         fprintf(stderr, "%s\n", bufferSig[i]);
                    //     }
                    //     free(bufferSig);
                    //     bufferWaiting=0;
                    //     bufferCount = 0;
                    // }
                } 
                fgpid = 1;
                free(cmd);
                return;
            }
            free(cmd);
            return;
        }
        free(cmd);
        return;
    }
    
    // check for JOBS builtin
    if(strcmp("jobs", cmd -> commands[0][0]) == 0){
        // if head null, print no jobs found
        if(head == NULL){
            char argbuf[strlen("No jobs present in the queue \n")+1];
            sprintf(argbuf, "%s \n", "No jobs present in the queue\n");
            f_write(PSTDOUT_FILENO, argbuf, strlen("No jobs present in the queue \n") + 1);
            // fprintf(stderr, "No jobs present in the queue\n");
            free(cmd);
            return;
        } 
        else {
            struct Job *current = head;
            int noBg = 0;
            do{
                if(current -> bgFlag == BG){
                    int len = strlen(current->commandInput);
                    if (len > 0 && current->commandInput[len - 1] == '\n') {
                        current->commandInput[len - 1] = '\0';
                    }
                    char argbuf[1024];
                    argbuf[0]='\0';
                    sprintf(argbuf, "[%d] %s (%s)\n", current -> JobNumber, current->commandInput, statusToStr(current -> status));
                    f_write(PSTDOUT_FILENO, argbuf, strlen(argbuf)+1);
                    // fprintf(stderr, "[%d] %s (%s)\n", current -> JobNumber, current->commandInput, statusToStr(current -> status));
                    noBg = 1;
                }
                current = current -> next;
            } while(current != NULL);
            
            if(noBg == 0){
                fprintf(stderr, "No bg jobs found\n");
            }
            free(cmd);
            return;
        }
    }

    // update priority via pid
    if(strcmp("nice_pid", cmd -> commands[0][0]) == 0){
        if(p_nice(atoi(cmd->commands[0][2]), atoi(cmd->commands[0][1])) == -1){
            fprintf(stderr, "Error: Process not found");
        }
        return;
    }
    
    int n = cmd -> num_commands;  
    if (cmd -> is_background){
        IS_BG = 1; 
        char argbuf[1024];
        argbuf[0]='\0';
        sprintf(argbuf, "Running:");
        f_write(PSTDOUT_FILENO, argbuf, strlen(argbuf) + 1);
        print_parsed_command(cmd);    
    }

    int status = 0;

    // create process via priority
    if(strcmp("nice", cmd -> commands[0][0]) == 0){
        shellBFunc(cmd);
        if(p_nice(curr_pid, atoi(cmd->commands[0][1])) == -1){
            fprintf(stderr, "Error: Process not found\n");
        }
    } else {
        shellBFunc(cmd);
    }

    if(retFlag)
        return;

    // for loop to execute the commands line by line
    struct Job *new_job = NULL; // create a new job each time penn shredder is run

    if(IS_BG == 1){
        // for the first process in the job, add everything
        new_job = createJob(curr_pid, BG, n, buffer); 
    }
    else{ // same for FG
        // for the first process in the job, add everything
        new_job = createJob(curr_pid, FG, n, buffer);  
    }
     
    if (IS_BG == 0){ // wait as normal for foreground processes
        // static sigset_t mask;
        // printf("FG process detected \n");
        // tc set to fg
        fgpid = curr_pid; // use for signal handling, and identifying the process in the foreground
        
        newpid = p_waitpid(curr_pid, &status, FALSE); 

        // sigprocmask(SIG_UNBLOCK, &mask, NULL);
        if (W_WIFSTOPPED(status) && new_job -> status == RUNNING){
            char argbuf[1024];
            argbuf[0]='\0';
            sprintf(argbuf, "\nStopped: %s\n", new_job->commandInput);
            f_write(PSTDOUT_FILENO, argbuf, strlen(new_job->commandInput)*3-2);
            f_write(PSTDOUT_FILENO, "\n", strlen("\n"));
            // fprintf(stderr, "\nStopped: %s", new_job-> commandInput); 
            new_job -> status = STOPPED; 
            new_job->bgFlag = 1;
            head = addJob(head, new_job);    
        }
    
        // tc set back to pennshell
        fgpid = 1;
        
        //print bufferSig here IF not empty
        // once (if) printed, empty it
        // if (bufferWaiting){
        //     //PRINT BUFFER
        //     for (int i = 0; i < bufferCount ; i++) {
        //         printf("%s\n", bufferSig[i]);
        //     }
        //     free(bufferSig);
        //     bufferWaiting=0;
        // }
    }

    // add the background job ALWAYS
    if(IS_BG){
        // printf("in BG, adding \n");
        head = addJob(head, new_job);
    }
    free(cmd);
    return;
}

void pennShell(){

    char buffer[INPUT_SIZE];

    // catch and ignore this signal else it does let jobs be suspended properly
    signal(SIGTTOU, SIG_IGN);
    
    par_pid = getpid();

    // create a jobs linked list 
    struct Job *current = NULL;

    flag_spaces = 0;
    while (1) {
        // Interactive Section (Penn Shredder: Normal)
        // Reading I/P here but polling here
        // POLLING 
        // int count = 0;
        int status;
        // int num = 0;
        
        if(isatty(fileno(stdin))){
            // WRITE AND READ 
            if (f_write(PSTDOUT_FILENO, PROMPT, sizeof(PROMPT)) == -1) {
                p_perror("write");
                // freeAllJobs(head);
                // freeAllJobs(current);
                p_exit();
            }

            int numBytes = f_read(PSTDIN_FILENO,  4096, buffer);

            if (numBytes == -1) {
                p_perror("read");
                p_kill(fgpid, S_SIGTERM);
                freeAllJobs(head);
                freeAllJobs(current);
                // exit(EXIT_FAILURE);
            }

            // loop through list

            while(1){

                if(head == NULL){
                    break;
                }
                pid_t pid = p_waitpid(-1, &status, TRUE);
                
                if (pid < 0){
                    break;
                }
    
                current = head;
                
                // iterateShell(head);
                do {
                    if(current->myPid == pid){
                        // printf("PID IS: %d\n",current->myPid);
                        break;
                    }
                    current = current->next;
                } while(current != NULL);               

                if (W_WIFSTOPPED(status) && current->status == RUNNING){
                    // pkill
                    char argbuf[strlen(current->commandInput) + 1];
                    // printf("Stopped hacky: %s\n", current->commandInput);
                    sprintf(argbuf, "Stopped: %s\n", current->commandInput); 
                    f_write(PSTDOUT_FILENO, argbuf, strlen(current->commandInput) + 1);
                    current -> status = STOPPED; 
                    // if (bufferWaiting){
                    //     //PRINT BUFFER
                    //     for (int i = 0; i < bufferCount ; i++) {
                    //         printf("%s\n", bufferSig[i]);
                    //     }
                    //     free(bufferSig);
                    //     bufferWaiting = 0;
                    //     bufferCount = 0;
                    // }
                }
                else if(W_WIFSIGNALED(status) && current -> status == RUNNING){
                    head = removeJob(head, current->JobNumber);
                    // if (bufferWaiting){
                    //     //PRINT BUFFER
                    //     for (int i = 0; i < bufferCount ; i++) {
                    //         printf("%s\n", bufferSig[i]);
                    //     }
                    //     free(bufferSig);
                    //     bufferWaiting = 0;
                    //     bufferCount = 0;
                    // }
                }
                else if(W_WIFEXITED(status) && current -> status == RUNNING){
                    char argbuf[4096];
                    // char str [4096];
                    // strcat(str,"Finished:");
                    sprintf(argbuf, "Finished: %s\n", current->commandInput); 
                    f_write(PSTDOUT_FILENO, argbuf, 4096);
                    head = removeJob(head, current->JobNumber);
                    
                    // if (bufferWaiting){
                    //     //PRINT BUFFER
                    //     for (int i = 0; i < bufferCount ; i++) {
                    //         printf("%s\n", bufferSig[i]);
                    //     }
                    //     free(bufferSig);
                    //     bufferWaiting = 0;
                    //     bufferCount = 0;
                    // }
                }
            }

            // Exit iteration if only new line given
            if (numBytes == 1 && buffer[0] == '\n') {
                continue;
            }
            // check if buffer is all spaces or tabs
            for (int i = 0; i < numBytes - 1; i++) {
                if (buffer[i] != ' ' && buffer[i] != '\t') {
                    break;
                }
                if (i == numBytes - 2) {
                    flag_spaces = 1;
                }
            }

            // this check and continue done outside for, else it just goes to the next iter of the for loop
            if (flag_spaces){
                (f_write(PSTDOUT_FILENO, "\n", strlen("\n")));
                continue;
            }    
                
            buffer[numBytes] = '\0'; // Set last char of buffer to null to prevent memory leaks
            
            // If no input or there is input but the last char of the buffer isn't newline, its CTRL D
            if (numBytes == 0 || (numBytes != 0 && buffer[numBytes - 1] != '\n')) {
                // printf("entered 1st line ctrld \n");
                if (numBytes == 0) { // In this case, just return a new line (avoids the # sign on the same line)
                    // printf("entered 2rd line ctrld \n");
                    if (f_write(PSTDOUT_FILENO, "\n", strlen("\n")) == -1) {
                        // printf("entered 3rd line ctrld \n");
                        p_perror("write");
                        // freeAllJobs(head);
                        // freeAllJobs(current);
                        p_exit();
                    }  
                    break; // Either ways, just shut the code
                }
                else{ // Normal case
                    if (f_write(PSTDOUT_FILENO, "\n", strlen("\n")) == -1) {
                        p_perror("write");
                        // freeAllJobs(head);
                        // freeAllJobs(current);
                        p_exit();
                    }  
                }
            }
            // printf("enterring shredd \n");
            pennShredder(buffer);
            if(head != NULL && current == NULL){
                current = head; // first job
            }   
        }
        // Non-interactive Section (Read from file)
        else{
            char *line = NULL; 
            size_t len = 0; // unsigned int type
            int numBytes = getline(&line, &len, stdin); // read line from txt file
            if (numBytes == -1) {
                // freeAllJobs(head);
                // freeAllJobs(current);
                exit(1);
            }
            // Exit iteration if only new line given
            if (numBytes == 1 && line[0] == '\n') {
                continue;
            }
            pennShredder(line);
            free(line);
        }
    }   
    // printf("outside shell while \n");
    // freeAllJobs(current);
    // freeAllJobs(head);
    free(bufferSig); 
}  