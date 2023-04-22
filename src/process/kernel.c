#include "kernel.h"

void setStack(stack_t *stack){
    void *sp = malloc(SIGSTKSZ);
    // Needed to avoid valgrind errors
    VALGRIND_STACK_REGISTER(sp, sp + SIGSTKSZ);

    *stack = (stack_t){.ss_sp = sp, .ss_size = SIGSTKSZ};
}

struct pcb* k_process_create(struct pcb *parent) {
    // Create a new process and set its ID and priority level
    
    // printf("Inside k_proc_create \n");

    ucontext_t *uc = (ucontext_t*) malloc(sizeof(ucontext_t));
    getcontext(uc);
    sigemptyset(&uc->uc_sigmask);

    setStack(&uc->uc_stack);
    uc->uc_link = &terminateContext;
    
    // Process *newProcess = (Process*) malloc(sizeof(Process));
    struct pcb *p = NULL;
                                                            
    p = createPcb(*uc, pidCounter, parent->pid, 0, RUNNING);
    
    pidCounter++;

    return p;
}

int k_process_kill(Process *p, int signal){

    // printf("inside k  process kill \n");
    switch (signal){
    case S_SIGTERM: {
        // printf("SIGTERM k_kill\n");
        char buf[50];
        buf[0] = '\0';
        char s[50];
        sprintf(s, "[%d]\t", ticks);
        strcat(buf, s);
        strcat(buf,"SIGNALED\t");
        sprintf(s, "%d\t", p->pcb->pid);
        strcat(buf,s);
        sprintf(s, "%d\t", p->pcb->priority);
        strcat(buf,s);
        sprintf(s, "%s", p->pcb->argument);
        strcat(buf,s);
        strcat(buf,"\n");
        writeLogs(buf);


        if(p->pcb->numChild == 0){
            // printf("SIGTERM 1\n");
            // printf("kproc kill 0 child %d \n", p->pcb->status);
            if(p->pcb->status == BLOCKED){
                Process *parent = findProcessByPid(p->pcb->ppid);
                dequeueBlocked(p);
                if(p->pcb->bgFlag == 1){
                    
                    enqueueZombie(p);
                    
                }
                dequeueBlocked(parent);

                char buf[50];
                buf[0] = '\0';
                char s[50];
                sprintf(s, "[%d]\t", ticks);
                strcat(buf, s);
                strcat(buf,"SCHEDULE\t");
                sprintf(s, "%d\t", parent->pcb->pid);
                strcat(buf,s);
                if (parent->pcb->priority == PRIORITY_HIGH)
                    sprintf(s, "%s\t", "HIGH");
                else if (parent->pcb->priority== PRIORITY_LOW)
                    sprintf(s, "%s\t", "LOW");
                else if (parent->pcb->priority == PRIORITY_MED)
                    sprintf(s, "%s\t", "MEDIUM");
                strcat(buf,s);
                if (parent->pcb->argument != NULL){
                    sprintf(s, "%s\t", parent->pcb->argument);
                    strcat(buf,s);
                }
                    
                strcat(buf,"\n");
                writeLogs(buf);
                enqueue(parent);
                // printf("kill BLOCKED\n");
            }
            else if(p->pcb->status == RUNNING){
                // printf("K KILL BEFORE FIND \n");
                // printf("Processes in BLOCKED Q\n");
                // iterateQueue(medQhead);
                Process *parent = findProcessByPid(p->pcb->ppid);
                // printf("K KILL after FIND \n");
                dequeue(p);
                if(p->pcb->bgFlag== 1){
                    enqueueZombie(p);
                }
                dequeueBlocked(parent);

                char buf[50];
                buf[0] = '\0';
                char s[20];
                sprintf(s, "[%d]\t", ticks);
                strcat(buf, s);
                strcat(buf,"SCHEDULE\t");
                sprintf(s, "%d\t", parent->pcb->pid);
                strcat(buf,s);
                if (parent->pcb->priority == PRIORITY_HIGH)
                    sprintf(s, "%s\t", "HIGH");
                else if (parent->pcb->priority== PRIORITY_LOW)
                    sprintf(s, "%s\t", "LOW");
                else if (parent->pcb->priority == PRIORITY_MED)
                    sprintf(s, "%s\t", "MEDIUM");
                strcat(buf,s);
                if (parent->pcb->argument != NULL){
                    sprintf(s, "%s\t", parent->pcb->argument);
                    strcat(buf,s);
                }
                    
                strcat(buf,"\n");
                writeLogs(buf);
                enqueue(parent);                
                // printf("kill RUNNING \n");
            }
            else if(p->pcb->status == STOPPED){
                dequeueStopped(p);
                // printf("kill STOPPED \n");
            }
            p->pcb->status = SIG_TERMINATED;
            p->pcb->changedStatus = 1;
            return 0;
        }
        for(int i=0; i < p->pcb->numChild; i++){
            // printf("SIGTERM %d\n", i);
            Process *child = findProcessByPid(p->pcb->childPids[i]);
            // enqueueOrphan(child);
            k_process_kill(child, S_SIGTERM);
        }
    }
    break;
    case S_SIGSTOP:{
        Process *parent = findProcessByPid(p->pcb->ppid);

        if(p->pcb->status == RUNNING){
            dequeue(p);
        }
        else if(p->pcb->status == BLOCKED){
            dequeueBlocked(p);
        }
        enqueueStopped(p);
        if (parent->pcb->status == BLOCKED){
            dequeueBlocked(parent);
            char buf[50];
            buf[0] = '\0';
            char s[50];
            sprintf(s, "[%d]\t", ticks);
            strcat(buf, s);
            strcat(buf,"SCHEDULE\t");
            sprintf(s, "%d\t", parent->pcb->pid);
            strcat(buf,s);
            if (parent->pcb->priority == PRIORITY_HIGH)
                sprintf(s, "%s\t", "HIGH");
            else if (parent->pcb->priority== PRIORITY_LOW)
                sprintf(s, "%s\t", "LOW");
            else if (parent->pcb->priority == PRIORITY_MED)
                sprintf(s, "%s\t", "MEDIUM");
            strcat(buf,s);
            if (parent->pcb->argument != NULL){
                sprintf(s, "%s\t", parent->pcb->argument);
                strcat(buf,s);
            }
                
            strcat(buf,"\n");
            writeLogs(buf);

            enqueue(parent);
        }
        p->pcb->status = STOPPED;
        p->pcb->changedStatus = 1;
        p->pcb->bgFlag = 1;
        return 0;
    }
    break;
    case S_SIGCONT:
        p->pcb->changedStatus = 0;
        if (p->pcb->status == STOPPED){
            dequeueStopped(p);
            if(fgpid == 1){ // if shell in foreground (bg command)
                // printf("Mark 3\n");
                p->pcb->bgFlag = 1;
                p->pcb->status = RUNNING;
                if(p->pcb->sleep_time_remaining != -1){
                    enqueueBlocked(p);
                } 
                else{
                    char buf[50];
                    buf[0] = '\0';
                    char s[50];
                    sprintf(s, "[%d]\t", ticks);
                    strcat(buf, s);
                    strcat(buf,"SCHEDULE\t");
                    sprintf(s, "%d\t", p->pcb->pid);
                    strcat(buf,s);
                    if (p->pcb->priority == PRIORITY_HIGH)
                        sprintf(s, "%s\t", "HIGH");
                    else if (p->pcb->priority== PRIORITY_LOW)
                        sprintf(s, "%s\t", "LOW");
                    else if (p->pcb->priority == PRIORITY_MED)
                        sprintf(s, "%s\t", "MEDIUM");
                    strcat(buf,s);
                    if (p->pcb->argument != NULL){
                        sprintf(s, "%s\t", p->pcb->argument);
                        strcat(buf,s);
                    }
                        
                    strcat(buf,"\n");
                    writeLogs(buf);
                    enqueue(p);
                }
            } else { // if shell not in foreground (fg command)
                p->pcb->bgFlag = 0;
                if(p->pcb->sleep_time_remaining != -1){ // if sleep has time left, block it and shell
                    enqueueBlocked(p);
                    p->pcb->changedStatus = 0;
                } 
                else{
                    p->pcb->status = RUNNING;
                    enqueue(p);
                }
            }
        } else if (p->pcb->status == BLOCKED && p->pcb->sleep_time_remaining >0){
            p->pcb->bgFlag = 0;
        } else if (p->pcb->status == RUNNING){
            p->pcb->bgFlag = 0;
            
        }
        // printf("Mark 5\n");
        break;
    default:
        return -1;
    }
    return -1;
}   

void k_process_cleanup(Process *p) { 

    
    if (p->pcb->status == BLOCKED){
        p->pcb->status = TERMINATED;
        p->pcb->changedStatus = 1;
        dequeueBlocked(p);
    }
    else{
        p->pcb->status = TERMINATED;
        p->pcb->changedStatus = 1;
        dequeue(p);
    }
    
    // printf("looking at %d in KPC\n", p->pcb->status);

    // printf("bg flag = %d\n", p->pcb->bgFlag);
    
    if(p->pcb->bgFlag == 1){
        // is background, so make zombie
        // printf("inside KPC about to enq zombie \n");
        enqueueZombie(p);
        return;
    }
    Process *temp = blockedQhead;

    // printf("temp %d\n", temp->pcb->pid);
    
    while(temp != NULL){
        if(temp->pcb->pid == p->pcb->ppid && (temp->pcb->waitChild == p->pcb->pid)){
            dequeueBlocked(temp);
            char buf[50];
            buf[0] = '\0';
            char s[50];
            sprintf(s, "[%d]\t", ticks);
            strcat(buf, s);
            strcat(buf,"SCHEDULE\t");
            sprintf(s, "%d\t", temp->pcb->pid);
            strcat(buf,s);
            if (temp->pcb->priority == PRIORITY_HIGH)
                sprintf(s, "%s\t", "HIGH");
            else if (temp->pcb->priority== PRIORITY_LOW)
                sprintf(s, "%s\t", "LOW");
            else if (temp->pcb->priority == PRIORITY_MED)
                sprintf(s, "%s\t", "MEDIUM");
            strcat(buf,s);
            if (temp->pcb->argument != NULL){
                sprintf(s, "%s\t", temp->pcb->argument);
                strcat(buf,s);
            }
                
            strcat(buf,"\n");
            writeLogs(buf);

            enqueue(temp);
            // printf("enq temp\n");
            break;
        }
        temp = temp->next;
    }

    if(p->pcb->numChild != 0){
        for(int i=0; i< p->pcb->numChild; i++){
            Process *cp = findProcessByPid(p->pcb->childPids[i]);
            k_process_kill(cp, S_SIGTERM);
        }
    }
    // printf("end of KPC\n");
    char buf[50];
    buf[0] = '\0';
    char s[50];
    sprintf(s, "[%d]\t", ticks);
    strcat(buf, s);
    strcat(buf,"EXITED\t");
    sprintf(s, "%d\t", p->pcb->pid);
    strcat(buf,s);
    sprintf(s, "%d\t", p->pcb->priority);
    strcat(buf,s);
    sprintf(s, "%s", p->pcb->argument);
    strcat(buf,s);
    strcat(buf,"\n");
    writeLogs(buf);
}

Process *findProcessByPid(int pid){

    Process *temp = highQhead;
    while(temp != NULL){
        if(temp->pcb->pid == pid){
            // printf("HIGH exiting find process pid\n");
            return temp;
        }
        temp = temp->next;
    }

    temp = medQhead;
    while(temp != NULL){
        // printf("pid of med q proc %d %d\n", temp->pcb->pid, pid);
        if(temp->pcb->pid == pid){
            // printf("MED exiting find process pid\n");
            return temp;
        }
        temp = temp->next;
    }

    temp = lowQhead;
    while(temp != NULL){
        if(temp->pcb->pid == pid){
            // printf("LOW exiting find process pid\n");
            return temp;
        }
        temp = temp->next;
    }

    temp = blockedQhead;
    while(temp != NULL){
        printf("pid of med q proc %d %d\n", temp->pcb->pid, pid);
        if(temp->pcb->pid == pid){
            printf("BLOCKED exiting find process pid\n");
            return temp;
        }
        temp = temp->next;
    }
    
    temp = stoppedQhead;
    while(temp != NULL){
        if(temp->pcb->pid == pid){
            printf("Stopped exiting find process pid %s\n", temp->pcb->argument);
            return temp;
        }
        temp = temp->next;
    }
    temp = zombieQhead;
    while(temp != NULL){
        if(temp->pcb->pid == pid){
            // printf("BLOCKED exiting find process pid with pid = %d\n", temp->pcb->pid);
            return temp;
        }
        temp = temp->next;
    }
    // printf("Didn't find the process I was looking for.\n");
    return NULL;
}