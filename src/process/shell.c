 #include "shell.h"

#define INPUT_SIZE 4096 // good programming practice

#define TRUE 1
#define FALSE 0

int ctrl_c = 0; // flag to check if ctrl c has been pressed
int flag_spaces = 0; // flag to note if the input buffer is all spaces and/or tabs
int timeout; // timeout arg to penn shredder, for extra credit to use alarm(timeout)
int IS_BG = 0;
int curr_pid = 0;
int par_pgid  = 0;
int bufferWaiting = 0;
int bufferCount;

void setTimer(void) {
    struct itimerval it;

    it.it_interval = (struct timeval){.tv_usec = quantum};
    it.it_value = it.it_interval;

    setitimer(ITIMER_REAL, &it, NULL);
}

void sigint_termHandler(int signal) {
    // ignore for bg processes
    if(signal == SIGINT){
        if(curr_pid != 0 && !IS_BG){
            kill(curr_pid, SIGKILL);
        }
    }
}

void sigcontHandler(int signal){

}

void sigtstpHandler(int signal){
    if(signal == SIGTSTP){
        if(curr_pid != 0 && !IS_BG){
            kill(curr_pid, SIGTSTP);
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

    sa_int.sa_handler = sigint_termHandler;
    sa_int.sa_flags = SA_RESTART;
    sigfillset(&sa_int.sa_mask);

    sigaction(SIGINT, &sa_int, NULL);

    struct sigaction sa_term;

    sa_term.sa_handler = sigint_termHandler;
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

struct Job *createJob(int pgid, int bgFlag, int numChildren, char *input){
    struct Job *newJob;
    newJob = (struct Job *)malloc(sizeof(struct Job));
    newJob -> commandInput = malloc((strlen(input) + 1) * sizeof(char));
    strcpy(input, newJob -> commandInput);
    newJob -> next = NULL;
    newJob -> numChild = numChildren;
    newJob -> bgFlag = bgFlag;
    newJob -> pgid = pgid;
    newJob -> status = RUNNING;
    newJob -> pids = malloc(numChildren * sizeof(int));
    newJob -> pids_finished = malloc(numChildren * sizeof(int));
    return newJob;
}

// clear all the mallocs to prevent memory leaks
void freeOneJob(struct Job *Job){
    free(Job -> commandInput);
    free(Job -> pids);
    free(Job -> pids_finished);
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
        return head;
    }
    
    // If there is only one job currently, this will be job #2
    if (head -> next == NULL){
        head -> next = newJob;
        newJob -> JobNumber = 2;
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
        return head;
    }

    // if there is a gap in job numbers, fill in that gap and link both ends of the new job
    if (current -> next -> JobNumber - current -> JobNumber > 1){
        newJob -> JobNumber = current -> JobNumber + 1;
        current -> next = newJob;
        newJob -> next = current -> next;
        return head;
    }
    return head;
}

// Removes a job give a specifc job number.
struct Job *removeJob(struct Job *head, int jobNum){

    // if first job, set the new head to the next job and free head
    if (jobNum == 1){
        struct Job *newHead = head -> next;
        freeOneJob(head);
        return newHead;
    }

    // iterate through all jobs until job of interest is reached
    struct Job *current= head;
    while (current -> next != NULL){

        // if the next job is the one, replace next with the one after that
        if (current -> next -> JobNumber == jobNum){
            struct Job *removed = current -> next;
            struct Job *newNext = removed -> next;
            current -> next = newNext;
            removed -> next = NULL;
            freeOneJob(removed);
            return head;
        }
        current = current -> next;
    }
    return head;
}

void pennShell(){
    printf("Inside penn shell \n");
    timeout = 0;

    // run the two signals
    // if(signal(SIGALRM, sigalarm_handler) == SIG_ERR){
    //     perror("signal");
    //     exit(EXIT_FAILURE);
    // } 
    
    // if(signal(SIGINT, sigint_handler) == SIG_ERR){
    //     perror("signal ctrl");
    //     exit(EXIT_FAILURE);
    // }

    while (1) {
        
        // WRITE AND READ 
        int write1 = f_write(PSTDOUT_FILENO, PROMPT, sizeof(PROMPT));
        if (write1 == -1) {
            p_perror("f_write");
            exit(EXIT_FAILURE); // replace with p_exit
        }

        char buffer[INPUT_SIZE];

        int numBytes = f_read(PSTDIN_FILENO, INPUT_SIZE, buffer);
        if (numBytes == -1) {
            p_perror("f_read");
            exit(EXIT_FAILURE);
        }

        // exit iterationg if only next line given
        if (numBytes == 1 && buffer[0] == '\n') {
            continue;
        }

        // set last char of buffer to null to prevent memory leaks
        buffer[numBytes] = '\0'; 
        
        // if no input or there is input but the last char of the buffer isn't newline, its CTRL D
        if (numBytes == 0 || (numBytes != 0 && buffer[numBytes - 1] != '\n')) {
            if (numBytes == 0) { // in this case, just return a new line (avoids the # sign on the same line)
                int write7 = f_write(PSTDOUT_FILENO, "\n", strlen("\n"));
                if (write7 == -1) {
                    p_perror("f_write");
                    exit(EXIT_FAILURE);
                }  
                break; // either ways, just shut the code
            }
            else{ // normal case
                int write6 = f_write(PSTDIN_FILENO, "\n", strlen("\n"));
                if (write6 == -1) {
                    p_perror("f_write");
                    exit(EXIT_FAILURE);
                }  
            }
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
            continue;
        }       

        // if the last char not a newline, reiterate and reprompt
        if (buffer[numBytes - 1] != '\n') {
            continue; // check if it should continue or
        }
        
        buffer[numBytes - 1] = '\0'; // avoid memory leaks

        struct parsed_command *cmd; 
        parse_command(buffer, &cmd);

        if (strcmp(cmd->commands[0][0], "sleep") == 0){
            curr_pid = p_spawn(sleepFunc, cmd -> commands[0], PSTDIN_FILENO, PSTDOUT_FILENO);
        }

        else if (strcmp(cmd->commands[0][0], "echo") == 0){
            curr_pid = p_spawn(echoFunc, cmd -> commands[0], PSTDIN_FILENO, PSTDOUT_FILENO);
        }

    //     if (pid == -1) {
    //         perror("fork");
    //         // free(argsv);
    //         fflush(stdin);
    //         exit(EXIT_FAILURE);
    //     }
        
    //     if (pid) { // child process has PID 0 (returned from the fork process), while the parent will get the actual PID of child
  
    //         //setcontext
    //         printf("In child rn yo \n");

    //         fflush(stdin);
    //         exit(EXIT_FAILURE); // exits child process and reprompts "penn-shredder"   
    //     }
    //     // parent section
    //     else {
    //         alarm(timeout); // set an alarm for "timeout" number of seconds (signal function catches SIGALRM)
    //         int wstatus;
    //         int waitCheck = wait(&wstatus); // wait for the child to complete execution
    //         if (waitCheck == -1) {
    //             perror("wait");
    //             fflush(stdin);
    //             exit(EXIT_FAILURE);
    //         }
            
    //         printf("In parent rn yo \n");
    //     }
    //     fflush(stdin);
    //     alarm(0); // reset alarm to 0 incase child process did not need to be killed, as alarm continues to run otherwise
    }   
}