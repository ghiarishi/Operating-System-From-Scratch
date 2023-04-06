#include "shell.h"

#define INPUT_SIZE 4096

char **bufferSig;

int async = 0;
int IS_BG = 0;
int pgid = 0;
int curr_pid = 0;
int par_pgid  = 0;
int bufferWaiting = 0;
int bufferCount;

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

// clear all the mallocs to prevent memory leaks
void freeOneJob(struct Process *proc){
    free(proc->pcb->argument);
    free(proc->pcb);
    free(proc);
}

// call freeOneJob for every job in order to clear the entire LL memory
void freeAllJobs(struct Process *head) {
    // if the head is null, nothing to clear
    if (head == NULL) {
        return;
    }

    // iterate through LL, call freeOneJob
    struct Process *current = head;
        while (current != NULL) {
            struct Process *removed = current;
            current = current->next;
            freeOneJob(removed);
    }
}

// input parameters: head of LL, newJob we want to add to LL
struct Process *addJob(struct Process *head, struct Process *newProcess){

    // if there are no Processes in the LL yet, create one, assign #1
    if (head == NULL){ 
        head = newProcess; 
        newProcess -> pcb -> jobID = 1;
        return head;
    }
    
    // If there is only one Process currently, this will be Process #2
    if (head -> next == NULL){
        head -> next = newProcess;
        newProcess -> pcb -> jobID = 2;
        return head;
    }

    // if Process #1 has been removed, the head will point to a Process with a number greater than 1. So add the new Process as Process #1. 
    if (head -> pcb -> jobID > 1) { 
        newProcess -> pcb -> jobID = 1;
        newProcess -> next = head;
        return newProcess;
    }

    struct Process *current = head -> next;

    // check if the difference in Process numbers through the LL continues to be 1
    while (current -> next != NULL && current -> next -> pcb -> jobID - current -> pcb -> jobID == 1){
        current = current -> next;
    }

    // Adding the Process to the end of the linked list since the last Process points to null
    if (current -> next == NULL){
        newProcess -> pcb -> jobID = current -> pcb -> jobID + 1;
        current -> next = newProcess;
        newProcess -> next = NULL;
        return head;
    }

    // if there is a gap in job numbers, fill in that gap and link both ends of the new job
    if (current -> next -> pcb -> jobID - current -> pcb -> jobID > 1){
        newProcess -> pcb -> jobID = current -> pcb -> jobID + 1;
        current -> next = newProcess;
        newProcess -> next = current -> next;
        return head;
    }
    return head;
}

// SCHEDULER WAALA
// Function to add a thread to the appropriate priority queue
void enqueue(struct Process* newProcess) {
    // Determine the appropriate priority queue based on the ProcessnewProcess's priority level
    switch(newProcess->pcb->priority) {
        case PRIORITY_HIGH:
            addJob(highQhead, newProcess);
            break;
        case PRIORITY_LOW:
            addJob(lowQhead, newProcess);
            break;
        default:
            addJob(medQhead, newProcess);
            break;
    }
}

// Removes a job give a specifc job number.
struct Process *removeJob(struct Process *head, int jobNum){

    // if first job, set the new head to the next job and free head
    if (jobNum == 1){
        struct Process *newHead = head -> next;
        freeOneJob(head);
        return newHead;
    }

    // iterate through all jobs until job of interest is reached
    struct Process *current= head;
    while (current -> next != NULL){
        // if the next job is the one, replace next with the one after that
        if (current -> next -> pcb -> jobID == jobNum){
            struct Process *removed = current -> next;
            struct Process *newNext = removed -> next;
            current -> next = newNext;
            removed -> next = NULL;
            freeOneJob(removed);
            return head;
        }
        current = current -> next;
    }
    return head;
}

void dequeue(struct Process *proc){
    switch(proc->pcb->priority) {
        case PRIORITY_HIGH:
            removeProcess(highQhead, proc->pcb->jobID);
            break;
        case PRIORITY_LOW:
            removeProcess(lowQhead, proc->pcb->jobID);
            break;
        default:
            removeProcess(medQhead, proc->pcb->jobID);
            break;
    }
}

struct Process *getJob(struct Process *head, int jobNum){
    if (jobNum == 1){
        return head;
    }
    // iterate through all jobs until job of interest is reached
    struct Process *current = head;
    while (current -> next != NULL){
        current = current -> next;
        // if the next job is the one, replace next with the one after that
        if (current -> pcb->jobID == jobNum){
            return current;
        }
    }
    freeOneJob(current); 
    fprintf(stderr,"No job with this ID found\n");
    exit(EXIT_FAILURE);
}

int getCurrentJob(struct Process *head){
    if(head -> next == NULL){
        return head -> pcb -> jobID;
    }
    int bgNum = 0;
    int stpNum = 0;
    // iterate through all jobs until job of interest is reached
    struct Process *current = head;
    do{
        // if the next job is the one, replace next with the one after that
        if(current ->pcb->status == STOPPED){
            stpNum = current -> pcb->jobID;
        }
        else if(current -> pcb -> bgFlag == BG){
            bgNum = current->pcb->jobID;
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
        exit(EXIT_FAILURE);
    }
}

void changeStatus(struct Process *head, int jobNum, int newStatus){

    if (jobNum == 1){
        if(newStatus == 0){
            head -> pcb -> status = TERMINATED;
        }
        else if (newStatus == 1){
            head -> pcb -> status = READY;
        }
        else if (newStatus == 2){
            head -> pcb -> status = RUNNING;
        }
        else{
            head -> pcb -> status = STOPPED;
        }
    }

    // iterate through all jobs until job of interest is reached
    struct Process *current = head;
    while (current -> next != NULL){
        current = current -> next;
        // if the next job is the one, replace next with the one after that
        if (current -> pcb->jobID == jobNum){
            if(newStatus == 0){
                head -> pcb -> status = TERMINATED;
            }
            else if (newStatus == 1){
                head -> pcb -> status = READY;
            }
            else if (newStatus == 2){
                head -> pcb -> status = RUNNING;
            }
            else{
                head -> pcb -> status = STOPPED;
            }
        }
    }
}

void changeFGBG(struct Process *head, int jobNum, int newFGBG){
    if (jobNum == 1){
        if(newFGBG == 0){
            head -> pcb->bgFlag = FG;
        }
        else{
            head -> pcb->bgFlag = BG;
        } 
    }
    // iterate through all jobs until job of interest is reached
    struct Process *current = head;
    while (current -> next != NULL){
        current = current -> next;
        if (current -> pcb->jobID == jobNum){
            if(newFGBG == 0){
                current -> pcb -> bgFlag = FG;
            }
            else{
                current -> pcb -> bgFlag = BG;
            }
        }
    }
}

char *statusToStr(int status){
    if(status == 0){
        return "terminated";
    }
    else if(status == 1){
        return "ready";
    }
    else if(status == 2){
        return "running";
    }
    else{
        return "stopped";
    }
}

struct Process *head = NULL;

/// @brief 
/// @param signal 
void sig_handler(int signal) {

    // if (!async){
    //     if (write(STDERR_FILENO, "\n", sizeof("\n")) == -1) {
    //         perror("write");
    //         exit(EXIT_FAILURE);
    //     }   

    //     if (write(STDERR_FILENO, PROMPT, sizeof(PROMPT)) == -1) {
    //         perror("write");
    //         exit(EXIT_FAILURE);
    //     } 
    // }  

    // ignore for bg processes
    if(signal == SIGINT){
        if(curr_pid != 0 && !IS_BG){
            killpg(pgid, SIGKILL);
            activeProcess = NULL;
            activeContext = &schedulerContext;
            setcontext(&activeContext);
        }
    }
    if(signal == SIGTSTP){
        if(curr_pid != 0 && !IS_BG){
            kill(curr_pid, SIGTSTP);
        }
    }

    if(signal == SIGCHLD && async){

        static sigset_t mask; //oldmask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD); 
        sigprocmask(SIG_BLOCK, &mask, NULL);
        
        int printflag = 0;

        if (getpgid(0) == par_pgid){
            printflag = 1; //no other process running in FG
        }

        struct Process *current = NULL;
        int bufferCount = 0;

        // count the number of jobs in the LL
        if (head != NULL){
            current = head;
            do{
                bufferCount ++;
                current = current -> next;
            } while(current != NULL);
        }

        int TERMINATEDIndices[bufferCount];

        bufferCount = bufferCount *2;

        bufferSig = malloc(bufferCount * sizeof(char *));

        // POLLING 
        int status;
        int num = 0;
        int num2 = 0;
        while(1){
            pid_t pid = waitpid(-1, &status, WNOHANG | WUNTRACED);
            if (pid <= 0){
                break;
            }
            if(head == NULL){
                break;
            }
            current = head;
            do{ 
                if (WIFSTOPPED(status) && current -> pcb -> status == RUNNING){  
                    if(printflag){
                        fprintf(stderr, "Stopped: %s\n", current->pcb -> argument);
                        printflag=0;
                        
                    }
                    else{
                        bufferSig[num2] = "Stopped: ";
                        bufferSig[num2 + 1] = current -> pcb -> argument;
                        num2 = num2 + 2;
                        bufferWaiting=1;
                    }
                    current -> pcb -> status = STOPPED;
                }
                else{
                    bool currJobTERMINATED = false;
                    
                    int n = current -> pcb -> numChild;

                    // check all pids in this job, if they match the returned pid, then mark TERMINATED
                    for(int i = 0; i < n; i++){
                        if(pid == current -> pcb -> pids[i]){
                            current -> pcb -> pidsFinished[i] = true; //  
                        }
                    }
                    
                    // check to see if all processes in current job are TERMINATED
                    for(int i = 0; i < n; i++){
                        if(current -> pcb -> pidsFinished[i] == false){
                            currJobTERMINATED = false;
                            break;
                        }
                        currJobTERMINATED = true;
                    }

                    // ONLY if all processes in this job are TERMINATED and its not ALREADY TERMINATED in the past, print TERMINATED: cmd
                    if(currJobTERMINATED && current -> pcb -> status == RUNNING){
                        if(printflag){
                            fprintf(stderr, "TERMINATED: %s\n", current->pcb -> argument);
                            printflag=0;
                        }
                        else{
                            bufferSig[num2] = "TERMINATED: ";
                            bufferSig[num2 + 1] = current -> pcb -> argument; 
                            num2 = num2 + 2;
                            bufferWaiting = 1;
                        }   
                        current -> pcb -> status = TERMINATED;          
                        TERMINATEDIndices[num] = current->pcb->jobID;
                        num ++;       
                    } 
                    if(currJobTERMINATED && current -> pcb -> status == STOPPED){
                        if(printflag){
                            fprintf(stderr, "TERMINATED: %s\n", current->pcb -> argument);
                            printflag=0;
                        }
                        else{
                            bufferSig[num2] = "TERMINATED: ";
                            bufferSig[num2 + 1] = current -> pcb -> argument; 
                            num2 = num2 + 2;
                            bufferWaiting = 1;
                        }   
                        current -> pcb -> status = TERMINATED;          
                        TERMINATEDIndices[num] = current -> pcb-> jobID;
                        num ++;       
                    } 
                }
                current = current -> next;
            } while(current != NULL);
        }

        // iterate through the TERMINATED job nums and remove them all
        for(int i = 0; i < num; i++){
            head = removeJob(head, TERMINATEDIndices[i]);
        }
        
        sigprocmask(SIG_UNBLOCK, &mask, NULL);
    }
}

void penn_shredder(char* buffer){
    IS_BG = 0;
    int numBytes = strlen(buffer);

    // exit iteration if only new line given
    if (numBytes == 1 && buffer[0] == '\n') {
        return;
    }

    buffer[numBytes] = '\0'; // set last char of buffer to null to prevent memory leaks
    
    struct parsed_command *cmd;
    int num = parse_command(buffer, &cmd);

    // error handling for parsed command
    switch(num){
        case 1: fprintf(stderr, "invalid: parser encountered an unexpected file input token '<' \n");
                break;
        case 2: fprintf(stderr, "invalid: parser encountered an unexpected file output token '>' \n");
                break;
        case 3: fprintf(stderr, "invalid: parser encountered an unexpected pipeline token '|' \n");
                break;
        case 4: fprintf(stderr, "invalid: parser encountered an unexpected ampersand token '&' \n");
                break;
        case 5: fprintf(stderr, "invalid: parser didn't find input filename following '<' \n");
                break;
        case 6: fprintf(stderr, "invalid: parser didn't find output filename following '>' or '>>' \n");
                break;
        case 7: fprintf(stderr, "invalid: parser didn't find any commands or arguments where it expects one \n");
                break;
    }

    if(num != 0){
        return;
    }
    
    // check for BG builtin
    if(strcmp("bg", cmd -> commands[0][0]) == 0){
        if(head == NULL){
            fprintf(stderr, "No jobs present in the queue \n");
            free(cmd);
            return;
        }
        
        // case where JID is given
        if(cmd -> commands[0][1] != NULL){
            int job_id = atoi(cmd -> commands[0][1]);
            struct Process *bgJob = getJob(head, job_id);
            if (bgJob -> pcb -> status == STOPPED){
                // Send a SIGCONT signal to the process to continue it in the background
                changeStatus(head, job_id, 0); // set job to running
                changeFGBG(head, job_id, 1); // set job to BG 
                fprintf(stderr,"Running: %s", bgJob -> pcb -> argument);
                killpg(bgJob -> pcb -> pgid, SIGCONT);
                free(cmd);
                return;
            } 
            else if (bgJob -> pcb -> status == RUNNING){
                changeFGBG(head, job_id, 1); // set job to BG 
                fprintf(stderr,"Running: %s", bgJob -> pcb -> argument);
                free(cmd);
                return;
            }  
        }
        else{
            // case where no job ID given
            int job_id = getCurrentJob(head);
            struct Process *bgJob = getJob(head, job_id);
            if (bgJob -> pcb -> status == STOPPED){
                // Send a SIGCONT signal to the process to continue it in the background
                changeStatus(head, job_id, 0); // set job to running
                changeFGBG(head, job_id, 1); // set job to BG 
                fprintf(stderr,"Running: %s", bgJob -> pcb -> argument);
                killpg(bgJob -> pcb -> pgid, SIGCONT);
                free(cmd);
                return;
            }
            else if(bgJob->pcb->status == RUNNING){
                changeFGBG(head, job_id, 1); // set job to BG 
                fprintf(stderr,"Running: %s", bgJob -> pcb -> argument);
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
            fprintf(stderr, "No jobs present in the queue \n");
            free(cmd);
            return;
        }
        
        // case where JID is given
        if(cmd -> commands[0][1] != NULL){
            int job_id = atoi(cmd -> commands[0][1]);
            struct Process *fgJob = getJob(head, job_id);
            if (fgJob -> pcb -> status == STOPPED){
                // Send a SIGCONT signal to the process to continue it in the background
                changeStatus(head, job_id, 0); // set job to running
                changeFGBG(head, job_id, 0); // set job to FG 
                fprintf(stderr,"Restarting: %s", fgJob -> pcb -> argument);
                killpg(fgJob->pcb->pgid, SIGCONT);
                tcsetpgrp(STDIN_FILENO, fgJob ->pcb-> pgid);
                int status;
                for (int i = 0; i < fgJob->pcb -> numChild; i++){
                    waitpid(fgJob->pcb->pids[i], &status, WUNTRACED);   
                }
                tcsetpgrp(STDIN_FILENO, getpgid(0)); // give TC to parent
                if(WIFSTOPPED(status)){ 
                    fprintf(stderr, "Stopped: %s\n", fgJob -> pcb -> argument); 
                    fgJob ->pcb -> status = STOPPED; 
                    if (bufferWaiting){
                        //PRINT BUFFER
                        for (int i = 0; i < bufferCount ; i++) {
                            fprintf(stderr,"%s\n", bufferSig[i]);
                        }
                        free(bufferSig);

                        bufferWaiting=0;
                        bufferCount = 0;
                    }
                }
                if(fgJob->pcb->status != STOPPED){
                    changeStatus(head, job_id, 2); // set job to TERMINATED
                    head = removeJob(head, fgJob -> pcb -> jobID);
                }
                free(cmd);
                return;
            }
            // not stopped, but running in BG
            else{
                tcsetpgrp(STDIN_FILENO, fgJob-> pcb-> pgid);
                changeFGBG(head, job_id, 0); // set job to FG 
                fprintf(stderr, "%s\n", fgJob -> pcb -> argument); 
                int status;
                for (int i = 0; i < fgJob->pcb-> numChild; i++){
                    waitpid(fgJob->pcb->pids[i], &status, WUNTRACED);   
                }
                tcsetpgrp(STDIN_FILENO, getpgid(0)); // give TC to parent
                if(WIFSTOPPED(status)){ 
                    fprintf(stderr, "Stopped: %s\n", fgJob-> pcb-> argument); 
                    fgJob->pcb-> status = STOPPED; 
                    if (bufferWaiting){
                        //PRINT BUFFER
                        for (int i = 0; i < bufferCount ; i++) {
                            fprintf(stderr, "%s\n", bufferSig[i]);
                        }
                        free(bufferSig);
                        bufferWaiting=0;
                        bufferCount = 0;
                    }
                }
                if(fgJob->pcb->status != STOPPED){
                    changeStatus(head, job_id, 2); // set job to TERMINATED
                    head = removeJob(head, fgJob -> pcb -> jobID);
                }
                free(cmd);
                return;
            }
        }
        else{ // case where no job ID given
            int job_id = getCurrentJob(head);
            struct Process *fgJob = getJob(head, job_id);
            if (fgJob->pcb-> status == STOPPED){
                // Send a SIGCONT signal to the process to continue it in the background
                changeStatus(head, job_id, 0); // set job to running
                changeFGBG(head, job_id, 0); // set job to FG 
                killpg(fgJob->pcb->pgid, SIGCONT);
                tcsetpgrp(STDIN_FILENO, fgJob->pcb -> pgid);
                fprintf(stderr, "Restarting: %s", fgJob -> pcb -> argument);
                int status; 
                for (int i = 0; i < fgJob->pcb -> numChild; i++){
                    waitpid(fgJob->pcb->pids[i], &status, WUNTRACED);   
                }
                tcsetpgrp(STDIN_FILENO, getpgid(0)); // give TC to parent
                if(WIFSTOPPED(status)){ 
                    fprintf(stderr, "Stopped: %s\n", fgJob -> pcb -> argument); 
                    fgJob->pcb-> status = STOPPED; 
                    if (bufferWaiting){
                        //PRINT BUFFER
                        for (int i = 0; i < bufferCount ; i++) {
                            fprintf(stderr, "%s\n", bufferSig[i]);
                        }
                        free(bufferSig);
                        bufferWaiting = 0;
                        bufferCount = 0;
                    }
                 }
                if(fgJob->pcb->status != STOPPED){
                    changeStatus(head, job_id, 2); // set job to TERMINATED
                    head = removeJob(head, fgJob->pcb->jobID);
                }
                free(cmd);
                return;
            }
            // not stopped, but running in BG
            else{
                tcsetpgrp(STDIN_FILENO, fgJob->pcb-> pgid);
                changeFGBG(head, job_id, 0); // set job to FG 
                fprintf(stderr,"%s\n", fgJob -> pcb -> argument); 
                int status;
                for (int i = 0; i < fgJob->pcb-> numChild; i++){
                    waitpid(fgJob->pcb->pids[i], &status, WUNTRACED);   
                }
                tcsetpgrp(STDIN_FILENO, getpgid(0)); // give TC to parent
                if(WIFSTOPPED(status)){ 
                    fprintf(stderr, "Stopped: %s\n", fgJob -> pcb -> argument); 
                    fgJob->pcb-> status = STOPPED; 
                    if (bufferWaiting){
                        //PRINT BUFFER
                        for (int i = 0; i < bufferCount ; i++) {
                            fprintf(stderr, "%s\n", bufferSig[i]);
                        }
                        free(bufferSig);
                        bufferWaiting=0;
                        bufferCount = 0;
                    }
                }
                if(fgJob->pcb->status != STOPPED){
                    changeStatus(head, job_id, 2); // set job to TERMINATED
                    head = removeJob(head, fgJob->pcb->jobID);
                }
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
            fprintf(stderr, "No jobs present in the queue \n");
            free(cmd);
            return;
        } 
        else {
            struct Process *current = head;
            int noBg = 0;
            do{
                if(current->pcb -> bgFlag == BG){
                    fprintf(stderr, "[%d] %s (%s)\n", current->pcb -> jobID, current->pcb -> argument, statusToStr(current->pcb -> status));
                    noBg = 1;
                }
                current = current-> next;
            } while(current != NULL);
            
            if(noBg == 0){
                fprintf(stderr, "No bg jobs found\n");
            }
            free(cmd);
            return;
        }
    }
    
    int n = cmd -> num_commands;
    int group_pid = 0;    
    if (cmd -> is_background){
        IS_BG = 1; 
        printf("Running: ");
        print_parsed_command(cmd);    
    }
    
    int fd[n - 1][2]; // Create file descriptors for all pipes
    
    int pid_list[n]; // To store a list of all child PIDs

    int status; 

    for (int i = 0; i < n-1; ++i) {
        pipe(fd[i]); //Create the pipes
    }

    // for loop to execute the commands line by line
    struct Process *new_job = NULL; // create a new job each time penn shredder is run

    for (int i = 0; i < n; ++i) { // n processes within one command line 1
    
        int pid = fork(); // create child process thats copy of the parent
        curr_pid = pid;

        if (pid == -1) {
            free(cmd);
            perror("fork"); //if error in forking
            exit(EXIT_FAILURE);
        }
        if (pid == 0) { // child process has PID 0 (returned from the fork process), while the parent will get the actual PID of child
            //Input redirection
            if (cmd -> stdin_file != NULL){
                //open file in read mode
                int fin = open(cmd -> stdin_file, O_RDONLY);
                if (fin < 0){
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                if (dup2(fin, STDIN_FILENO) < 0){ // read from fin instead of stdin file no
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
                close(fin);
            }

            //Output redirection
            if (cmd -> stdout_file != NULL){
                int file; //file descriptor
                if (cmd  ->  is_file_append){
                    file = open(cmd -> stdout_file, O_WRONLY | O_APPEND, 0644); //Append mode
                }
                else{
                    file = open(cmd -> stdout_file, O_WRONLY | O_CREAT | O_TRUNC, 0644); //Overwrite mode
                }
                if (file < 0){
                    perror("open");
                    free(cmd);
                    return;
                }
                else{
                    if (dup2(file, STDOUT_FILENO) < 0){
                        perror("dup2"); 
                    }
                    close(file);
                }
            }
            
            //Pipelining
            if (n > 1){ // first command
                if (i == 0){
                    if (dup2(fd[i][1],STDOUT_FILENO) < 0){
                        perror("dup2");
                    }
                    close(fd[i][0]); 
                    close(fd[i][1]); 
                }
                else if (i == n - 1){ // last command
                    if (dup2(fd[i-1][0],STDIN_FILENO) < 0){
                        perror("dup2");
                    }
                    close(fd[i-1][0]);
                    close(fd[i-1][1]);
                }
                else{ // any middle command - pipe in and pipe out
                    if (dup2(fd[i-1][0],STDIN_FILENO) < 0){
                        perror("dup2");
                    }
                    if (dup2(fd[i][1],STDOUT_FILENO) < 0){
                        perror("dup2");
                    }
                    close(fd[i][0]);   // close read end of current pipe
                    close(fd[i-1][1]); // close write end of previous pipe
                }
            }
            
            if (execvp(cmd -> commands[i][0], cmd -> commands[i]) == -1) { 
                free(cmd);
                perror("execvp");
                exit(EXIT_FAILURE);
            } 

            // Close all ends for pipes in child
            for (int j = 0; j < n-1; j++){
                close(fd[j][0]);
                close(fd[j][1]);  
            }  
            free(cmd);
            return;                
        }
        else{ // parent else
            pid_list[i] = pid; // Add child's PID to the list
            group_pid = pid_list[0]; // set the pid of the first child to be the pgid
            setpgid(pid, group_pid); // set for every child 
            pgid = group_pid;

            // Close all ends of pipes in parent
            if(i != 0){
                close(fd[i-1][0]);
                close(fd[i-1][1]);   
            }

            if(IS_BG == 1){
                // for the first process in the job, add everything
                if(i == 0){ 
                    new_job = createJob(group_pid, BG, n, buffer);
                }
                // do this for every process in the job
                new_job->pcb -> pids[i] = pid; 
                new_job->pcb -> pidsFinished[i] = false;                
            }
            else{ // same for FG
                // for the first process in the job, add everything
                if(i == 0){ 
                    new_job = createJob(group_pid, FG, n, buffer);
                }
                // do this for every process in the job
                new_job->pcb -> pids[i] = pid;
                new_job->pcb -> pidsFinished[i] = false;     
            }
        }        
    }
    
    if (IS_BG == 0){ // wait as normal for foreground processes
        // static sigset_t mask;

        tcsetpgrp(STDIN_FILENO, pid_list[0]); // give TC to child

        for (int i = 0; i < n; i++){
            waitpid(-group_pid, &status, WUNTRACED);   
        }
        // sigprocmask(SIG_UNBLOCK, &mask, NULL);
        if (WIFSTOPPED(status) && new_job->pcb -> status == RUNNING){
            fprintf(stderr, "\nStopped: %s", new_job-> pcb -> argument); 
            new_job->pcb -> status = STOPPED; 
            head = addJob(head, new_job);
            
        }
        else{
            freeOneJob(new_job);
        }
        tcsetpgrp(STDIN_FILENO, getpgid(0)); // give TC to parent
        //print bufferSig here IF not empty

        // once (if) printed, empty it
        if (bufferWaiting){
            //PRINT BUFFER
            for (int i = 0; i < bufferCount ; i++) {
                printf("%s\n", bufferSig[i]);
            }
            free(bufferSig);
            bufferWaiting=0;
        }
    }

    // add the background job ALWAYS
    if(IS_BG){
        head = addJob(head, new_job);
    }
    free(cmd);
    return;
}

int main(int argc, char** argv) {  
    if(argc != 1 && argc != 2 && argc != 3) {  // penn shell args: check validity
        freeAllJobs(head);
        exit(EXIT_FAILURE);
    }
    if(argc == 2 && strcmp(argv[1], "--async") == 0){
        async = 1;
    }
    char buffer[INPUT_SIZE];
    if(signal(SIGINT, sig_handler) == SIG_ERR){
        perror("signal ctrl");
        freeAllJobs(head);
        exit(EXIT_FAILURE);
    }
    if(signal(SIGTSTP, sig_handler) == SIG_ERR){
        perror("signal stp");
        freeAllJobs(head);
        exit(EXIT_FAILURE);
    }

    if(async){
        if(signal(SIGCHLD, sig_handler) == SIG_ERR){
            perror("signal stp");
            freeAllJobs(head);
            exit(EXIT_FAILURE);
        }
    }

    // catch and ignore this signal else it does let jobs be suspended properly
    signal(SIGTTOU, SIG_IGN);
    
    par_pgid = getpgid(0);

    // create a jobs linked list 
    struct Process *current = NULL;

    while (1) {
        // Interactive Section (Penn Shredder: Normal)
        // Reading I/P here but polling here
        // POLLING 
        int count = 0;
        int TERMINATEDIndices[count];
        int status;
        int num = 0;
        
        if(isatty(fileno(stdin))){
            // WRITE AND READ 
            if (write(STDERR_FILENO, PROMPT, sizeof(PROMPT)) == -1) {
                perror("write");
                freeAllJobs(head);
                freeAllJobs(current);
                exit(EXIT_FAILURE);
            }

            int numBytes = read(STDIN_FILENO, buffer, INPUT_SIZE);
            if (numBytes == -1) {
                perror("read");
                freeAllJobs(head);
                freeAllJobs(current);
                exit(EXIT_FAILURE);
            }

            while(1){
                pid_t pid = waitpid(-1, &status, WNOHANG | WUNTRACED);
                if (pid <= 0){
                    break;
                }
                if(head == NULL){
                    break;
                }
                current = head;
                do{ 
                    if (WIFSTOPPED(status) && current->pcb -> status == RUNNING){
                        printf("Stopped: %s\n", current -> pcb -> argument); 
                        current->pcb -> status = STOPPED; 
                        if (bufferWaiting){
                        //PRINT BUFFER
                        for (int i = 0; i < bufferCount ; i++) {
                            printf("%s\n", bufferSig[i]);
                        }
                        free(bufferSig);
                        bufferWaiting=0;
                        bufferCount = 0;
                        }
                    }
                    else{
                        bool currJobTERMINATED = false;
                        int n = current->pcb -> numChild;

                        // check all pids in this job, if they match the returned pid, then mark TERMINATED
                        for(int i = 0; i < n; i++){
                            if(pid == current->pcb -> pids[i]){
                                current->pcb -> pidsFinished[i] = true; 
                            }
                        }
                        
                        // check to see if all processes in current job are TERMINATED
                        for(int i = 0; i < n; i++){
                            if(current->pcb -> pidsFinished[i] == false){
                                currJobTERMINATED = false;
                                break;
                            }
                            currJobTERMINATED = true;
                        }

                        // ONLY if all processes in this job are TERMINATED and its not ALREADY TERMINATED in the past, print TERMINATED: cmd
                        if(currJobTERMINATED && current->pcb -> status == RUNNING){
                            char *command = current -> pcb -> argument;
                            printf("TERMINATED: %s\n", command);
                            current->pcb -> status = TERMINATED;          
                            TERMINATEDIndices[num] = current->pcb -> jobID;
                            num ++;       
                            if (bufferWaiting){
                                //PRINT BUFFER
                                for (int i = 0; i < bufferCount ; i++) {
                                    printf("%s\n", bufferSig[i]);
                                }
                                free(bufferSig);
                                bufferWaiting=0;
                                bufferCount = 0;
                            }
                        } 
                    }
                    current = current -> next;
                } while(current != NULL);
            }
        
            // count the number of jobs in the LL
            if (head != NULL){
                current = head;
                do{
                    count ++;
                    current = current -> next;
                }while(current != NULL);
            }

            // iterate through the TERMINATED job nums and remove them all
            for(int i = 0; i < num; i++){
                head = removeJob(head, TERMINATEDIndices[i]);
            }

            // Exit iteration if only new line given
            if (numBytes == 1 && buffer[0] == '\n') {
                continue;
            }
            
            buffer[numBytes] = '\0'; // Set last char of buffer to null to prevent memory leaks
            
            // If no input or there is input but the last char of the buffer isn't newline, its CTRL D
            if (numBytes == 0 || (numBytes != 0 && buffer[numBytes - 1] != '\n')) {
                if (numBytes == 0) { // In this case, just return a new line (avoids the # sign on the same line)
                    if (write(STDERR_FILENO, "\n", strlen("\n")) == -1) {
                        perror("write");
                        freeAllJobs(head);
                        freeAllJobs(current);
                        exit(EXIT_FAILURE);
                    }  
                    break; // Either ways, just shut the code
                }
                else{ // Normal case
                    if (write(STDERR_FILENO, "\n", strlen("\n")) == -1) {
                        perror("write");
                        freeAllJobs(head);
                        freeAllJobs(current);
                        exit(EXIT_FAILURE);
                    }  
                }
            }
            penn_shredder(buffer);
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
                freeAllJobs(head);
                freeAllJobs(current);
                exit(1);
            }
            // Exit iteration if only new line given
            if (numBytes == 1 && line[0] == '\n') {
                continue;
            }
            penn_shredder(line);
            free(line);
        }
    }   
    freeAllJobs(current);
    freeAllJobs(head);
    free(bufferSig);
    return 0;    
}  