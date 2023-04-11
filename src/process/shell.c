#include "shell.h"
#define TRUE 1
#define FALSE 0

#define INPUT_SIZE 4096 // good programming practice

int pid; // process ID
int ctrl_c = 0; // flag to check if ctrl c has been pressed
int flag_spaces = 0; // flag to note if the input buffer is all spaces and/or tabs
int timeout; // timeout arg to penn shredder, for extra credit to use alarm(timeout)

void sigint_handler(int signal) {
    int write2 = write(STDERR_FILENO, "\n", sizeof("\n"));
    if (write2 == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    if(!ctrl_c) { // basically, print the prompt when there is nothing written, DONT print the prompt when there is another command! 
        int write3 = write(STDERR_FILENO, PROMPT, sizeof(PROMPT));
        if (write3 == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }
    }
    ctrl_c = 0; // reset the flag for another iteration of the while loop
}

// // clear all the mallocs to prevent memory leaks
// void freeOneJob(struct Process *proc){
//     free(proc->pcb->argument);
//     free(proc->pcb);
//     free(proc);
// }

// // call freeOneJob for every job in order to clear the entire LL memory
// void freeAllJobs(struct Process *head) {
//     // if the head is null, nothing to clear
//     if (head == NULL) {
//         return;
//     }

//     // iterate through LL, call freeOneJob
//     struct Process *current = head;
//         while (current != NULL) {
//             struct Process *removed = current;
//             current = current->next;
//             freeOneJob(removed);
//     }
// }

// input parameters: head of LL, newJob we want to add to LL
struct Process *addJob(struct Process *head, struct Process *newProcess){

    // if there are no Processes in the LL yet, create one, assign #1
    if (head == NULL){ 
        head = newProcess; 
        printf("Making pennshell the head of high q\n");
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
    printf("Inside enqueue\n");
    switch(newProcess->pcb->priority) {
        case PRIORITY_HIGH:
            printf("Inside HighQ!\n");
            highQhead = addJob(highQhead, newProcess);
            break;
        case PRIORITY_LOW:
            printf("Inside LowQ!\n");
            lowQhead = addJob(lowQhead, newProcess);
            break;
        default:
            printf("Inside MedQ!\n");
            medQhead = addJob(medQhead, newProcess);
            break;
    }
}

// Removes a job give a specifc job number.
struct Process *removeJob(struct Process *head, int jobNum){

    // if first job, set the new head to the next job and free head
    if (jobNum == 1){
        struct Process *newHead = head -> next;
        // freeOneJob(head);
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
            // freeOneJob(removed);
            return head;
        }
        current = current -> next;
    }
    return head;
}

void dequeue(struct Process *proc){
    switch(proc->pcb->priority) {
        case PRIORITY_HIGH:
            removeJob(highQhead, proc->pcb->jobID);
            break;
        case PRIORITY_LOW:
            removeJob(lowQhead, proc->pcb->jobID);
            break;
        default:
            removeJob(medQhead, proc->pcb->jobID);
            break;
    }
}

void pennShell(){
    printf("Inside penn shell \n");
    timeout = 0;

    // run the two signals
    // if(signal(SIGALRM, sigalarm_handler) == SIG_ERR){
    //     perror("signal");
    //     exit(EXIT_FAILURE);
    // } 
    
    if(signal(SIGINT, sigint_handler) == SIG_ERR){
        perror("signal ctrl");
        exit(EXIT_FAILURE);
    }
    while (1) {
        
        // WRITE AND READ 
        fflush(stdout);

        // int f_write(int fd, const char *str, int n);


        int write1 = write(STDERR_FILENO, PROMPT, sizeof(PROMPT));
        if (write1 == -1) {
            perror("write");
            exit(EXIT_FAILURE); // replace with p_exit
        }

        char buffer[INPUT_SIZE];

        int numBytes = read(STDIN_FILENO, buffer, INPUT_SIZE);
        if (numBytes == -1) {
            perror("read");
            fflush(stdin);
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
                int write7 = write(STDERR_FILENO, "\n", strlen("\n"));
                if (write7 == -1) {
                    perror("write");
                    fflush(stdin);
                    exit(EXIT_FAILURE);
                }  
                break; // either ways, just shut the code
            }
            else{ // normal case
                int write6 = write(STDERR_FILENO, "\n", strlen("\n"));
                if (write6 == -1) {
                    perror("write");
                    fflush(stdin);
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

        // char *delimiters = " \n\t";
        
        buffer[numBytes - 1] = '\0'; // avoid memory leaks

        struct parsed_command *cmd; 
        parse_command(buffer, &cmd);

        if (strcmp(cmd->commands[0][0], "sleep") == 0){
            pid = p_spawn(*sleepFunc, cmd -> commands[0], 0, 1);
        }

        // pid = p_spawn(cmd->commands[0][0], cmd -> commands[0], cmd -> stdin_file, cmd -> stdout_file ); // create child process thats copy of the parent

        if (pid == -1) {
            perror("fork");
            // free(argsv);
            fflush(stdin);
            exit(EXIT_FAILURE);
        }
        
        if (pid) { // child process has PID 0 (returned from the fork process), while the parent will get the actual PID of child
  
            //setcontext
            printf("In child rn yo \n");

            fflush(stdin);
            exit(EXIT_FAILURE); // exits child process and reprompts "penn-shredder"   
        }
        // parent section
        else {
            alarm(timeout); // set an alarm for "timeout" number of seconds (signal function catches SIGALRM)
            int wstatus;
            int waitCheck = wait(&wstatus); // wait for the child to complete execution
            if (waitCheck == -1) {
                perror("wait");
                fflush(stdin);
                exit(EXIT_FAILURE);
            }
            
            printf("In parent rn yo \n");
        }
        fflush(stdin);
        alarm(0); // reset alarm to 0 incase child process did not need to be killed, as alarm continues to run otherwise
    }   
}