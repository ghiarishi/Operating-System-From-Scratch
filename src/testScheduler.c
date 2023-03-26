#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <ucontext.h>
#include "pcb.h"

#define STACK_SIZE 8192
#define INTERVAL 50000

typedef struct node {
    struct pcb *pcb;
    struct node *next;
} NODE;

NODE *ready_queue[3] = {NULL, NULL, NULL};
NODE *running_queue = NULL;

ucontext_t sched_context, main_context;

void mkcontext(ucontext_t *uc, (void (*)(void))function) {
    getcontext(uc);
    uc->uc_stack.ss_sp = malloc(STACK_SIZE);
    uc->uc_stack.ss_size = STACK_SIZE;
    uc->uc_link = &main_context;
    makecontext(uc, function, 0);
}

void scheduler() {
    while (1) {
        int priority = pick_thread();
        if (priority == -1) {
            mkcontext(&sched_context, (void *)clean_up_terminated);
        } else {
            NODE *next_node = ready_queue[priority];
            remove_node_from_ready_queue(next_node);
            pcb_t *next_pcb = next_node->pcb;
            add_node_to_ready_queue(running_queue, next_pcb->priority);
            mkcontext(&sched_context, (void *)context_switch);
        }
        swapcontext(&main_context, &sched_context);
    }
}

void context_switch() {
    if (running_queue == NULL) {
        mkcontext(&sched_context, (void *)scheduler);
    } else {
        pcb_t *current_pcb = running_queue->pcb;
        if (current_pcb->status == TERMINATED) {
            remove_node_from_ready_queue(running_queue);
            free(current_pcb);
            free(running_queue);
            running_queue = NULL;
            mkcontext(&sched_context, (void *)scheduler);
        } else if (current_pcb->remaining_time > 0) {
            current_pcb->remaining_time--;
            mkcontext(&sched_context, current_pcb->argument);
        } else {
            current_pcb->remaining_time = current_pcb->time_slot;
            remove_node_from_ready_queue(running_queue);
            add_node_to_ready_queue(ready_queue[current_pcb->priority], current_pcb->priority);
            mkcontext(&sched_context, (void *)scheduler);
        }
    }
    swapcontext(&main_context, &sched_context);
}

void clean_up_terminated() {
    NODE *current = ready_queue[0];
    while (current != NULL) {
        NODE *next = current->next;
        if (current->pcb->status == TERMINATED) {
            remove_node_from_ready_queue(current);
            free(current->pcb);
            free(current);
        }
        current = next;
    }
    current = ready_queue[1];
    while (current != NULL) {
        NODE *next = current->next;
        if (current->pcb->status == TERMINATED) {
            remove_node_from_ready_queue(current);
            free(current->pcb);
            free(current);
        }
        current = next;
    }
    current = ready_queue[2];
    while (current != NULL) {
        NODE *next = current->next;
        if (current->pcb->status == TERMINATED) {
            remove_node_from_ready_queue(current);
            free(current->pcb);
            free(current);
        }
        current = next;
    }
    mkcontext(&sched_context, (void *)scheduler);
    swapcontext(&main_context, &sched_context);
}

void timer_interrupt(int j, siginfo_t *si, void *old_context) {
    block_timer();
    add_up_time_interval();
    unblock_timer();
    mkcontext(&sched_context, (void *)scheduler);
    swapcontext(&main_context, &sched_context);
}

int add_node_to_ready_queue(NODE *node, int priority) {
    if (node == NULL) {
        return 0;
    }

    node->next = ready_queue[priority];
    ready_queue[priority] = node;
    node->pcb->status = READY;
    return 1;
}

int remove_node_from_ready_queue(NODE *node) {
    if (node == NULL) {
        return 0;
    }
    NODE *prev = NULL;
    NODE *current = ready_queue[node->pcb->priority];
    while (current != NULL) {
        if (current == node) {
            if (prev == NULL) {
                ready_queue[node->pcb->priority] = current->next;
            } else {
                prev->next = current->next;
            }
            node->next = NULL;
            return 1;
        }
        prev = current;
        current = current->next;
    }
    return 0;
}

int change_status(pcb_t *pcb, int status, int priority) {
    if (pcb == NULL) {
        return 0;
    }
    pcb->status = status;
    if (status == RUNNING) {
        NODE *node = (NODE *)malloc(sizeof(NODE));
        node->pcb = pcb;
        node->next = NULL;
        running_queue = node;
    } else if (status == READY) {
        NODE *node = (NODE *)malloc(sizeof(NODE));
        node->pcb = pcb;
        node->next = NULL;
        add_node_to_ready_queue(node, priority);
    }
    return 1;
}

void init_scheduler_pcb() {
    pcb_t *scheduler_pcb = (pcb_t *)malloc(sizeof(pcb_t));

    scheduler_pcb->pid = 0;
    scheduler_pcb->priority = 1;
    scheduler_pcb->status = RUNNING;
    scheduler_pcb->remaining_time = INTERVAL;

    mkcontext(&sched_context, (void *)scheduler);

    NODE *scheduler_node = (NODE *)malloc(sizeof(NODE));
    scheduler_node->pcb = scheduler_pcb;
    scheduler_node->next = NULL;
    running_queue = scheduler_node;
    pcb_t *idle_pcb = (pcb_t *)malloc(sizeof(pcb_t));
    idle_pcb->pid = 1;
    idle_pcb->priority = -1;
    idle_pcb->status = READY;
    idle_pcb->remaining_time = INTERVAL;
    NODE *idle_node = (NODE *)malloc(sizeof(NODE));
    idle_node->pcb = idle_pcb;
    idle_node->next = NULL;
    add_node_to_ready_queue(idle_node, idle_pcb->priority);
}

void block_timer() {
    sigset_t sigmask;
    sigemptyset(&sigmask);
    sigaddset(&sigmask, SIGALRM);
    sigprocmask(SIG_BLOCK, &sigmask, NULL);
}

void unblock_timer() {
    sigset_t sigmask;
    sigemptyset(&sigmask);
    sigaddset(&sigmask, SIGALRM);
    sigprocmask(SIG_UNBLOCK,&sigmask, NULL);
}

int pick_thread() {
    int i;
    NODE *node = NULL;
    for (i = 1; i >= -1; i--) {
    node = ready_queue[i];
    if (node != NULL) {
    ready_queue[i] = node->next;
    node->next = NULL;
    break;
    }
    }
    if (node == NULL) {
    node = (NODE *)malloc(sizeof(NODE));
    node->pcb = (pcb_t *)malloc(sizeof(pcb_t));
    node->pcb->pid = 1;
    node->pcb->priority = -1;
    node->pcb->status = READY;
    node->pcb->remaining_time = INTERVAL;
    }
    return node->pcb->pid;
}

void context_switch() {
    NODE *current = running_queue;
    NODE *next = (NODE *)malloc(sizeof(NODE));
    next->pcb = (pcb_t *)malloc(sizeof(pcb_t));
    next->pcb->pid = pick_thread();
    next->pcb->priority = current->pcb->priority;
    change_status(current->pcb, READY, current->pcb->priority);
    change_status(next->pcb, RUNNING, next->pcb->priority);
    next->next = running_queue;
    running_queue = next;
    swapcontext(&(current->pcb->context), &(next->pcb->context));
}

int main() {
    init_scheduler_pcb();
    setup_signals();
    while (1) {
    if (running_queue->pcb->status == TERMINATED) {
    clean_up_terminated();
    }
    context_switch();
    }
    return 0;
}

void clean_up_terminated() {
    NODE *current = running_queue;
    pcb_t *p = current->pcb;
    remove_node_from_ready_queue(p->priority);
    free(p->context.uc_stack.ss_sp);
    free(p->context.uc_link);
    free(p);
    free(current);
    context_switch();
}

void add_up_time_interval() {
    NODE *node = running_queue;
    node->pcb->remaining_time -= INTERVAL;
    if (node->pcb->remaining_time <= 0) {
        remove_node_from_ready_queue(node->pcb->priority);
        node->pcb->remaining_time = INTERVAL;
        add_node_to_ready_queue(node, node->pcb->priority);
        context_switch();
    }
}

void block_timer() {
    struct itimerval new;
    new.it_interval.tv_sec = 0;
    new.it_interval.tv_usec = 0;
    new.it_value.tv_sec = 0;
    new.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &new, NULL);
}

void unblock_timer() {
    struct itimerval new;
    new.it_interval.tv_sec = 0;
    new.it_interval.tv_usec = INTERVAL * 1000;
    new.it_value.tv_sec = 0;
    new.it_value.tv_usec = INTERVAL * 1000;
    setitimer(ITIMER_REAL, &new, NULL);
}

int remove_node_from_ready_queue(int priority) {
    NODE *current = ready_queue[priority];
    NODE *prev = NULL;
    while (current != NULL) {
        if (current->pcb->pid == running_queue->pcb->pid) {
            if (prev == NULL) {
                ready_queue[priority] = current->next;
            } else {
                prev->next = current->next;
            }
            return 1;
        }
        prev = current;
        current = current->next;
    }
    return 0;
}

int add_node_to_ready_queue(NODE *node, int priority) {
    if (ready_queue[priority] == NULL) {
        ready_queue[priority] = node;
        return 1;
    } else {
        NODE *current = ready_queue[priority];
        NODE *prev = NULL;
        while (current != NULL) {
            if (node->pcb->priority > current->pcb->priority) {
                if (prev == NULL) {
                ready_queue[priority] = node;
                } else {
                prev->next = node;
                }
                node->next = current;
                return 1;
            }
            prev = current;
            current = current->next;
        }
        prev->next = node;
        return 1;
    }
    return 0;
}

int change_status(pcb_t *pcb, int status, int priority) {
    switch (status) {
        case RUNNING:
        pcb->status = RUNNING;
        add_node_to_ready_queue(running_queue, running_queue->pcb->priority);
        running_queue = (NODE *)malloc(sizeof(NODE));
        running_queue->pcb = pcb;
        break;
        case READY:
        pcb->status = READY;
        add_node_to_ready_queue(running_queue, running_queue->pcb->priority);
        break;
        case TERMINATED:
        pcb->status = TERMINATED;
        break;
        default:
        break;
    }
    return 0;
}

void f(){
    printf("hellooo worllssss");
}

void init_scheduler_pcb() {
    NODE *node = (NODE *)malloc(sizeof(NODE));
    node->pcb = (pcb_t *)malloc(sizeof(pcb_t));
    node->pcb->pid = 0;
    node->pcb->priority = -1;
    node->pcb->status = RUNNING;
    node->pcb->remaining_time = INTERVAL;
    running_queue = node;
    ready_queue[-1] = NULL;
    ready_queue[0] = NULL;
    ready_queue[1] = NULL;

    node = (NODE *)malloc(sizeof(NODE));
    node->pcb = (pcb_t *)malloc(sizeof(pcb_t));
    node->pcb->pid = 1;
    node->pcb->priority = -1;
    node->pcb->status = READY;
    node->pcb->remaining_time = INTERVAL;
    mkcontext(&node->pcb->context, f());
    add_node_to_ready_queue(node, node->pcb->priority);
}

int main() {
    setup_scheduler();
    kill(0, SIGALRM);
    while (1);
    return 0;
}


