#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#define STACK_SIZE 8192
#define QUANTUM 3

ucontext_t context_sleep, context_echo;

void sleep_func() {
    printf("Sleeping for 6 seconds...\n");
    sleep(6);
    printf("Finished sleeping\n");
    setcontext(&context_echo);
}

void echo_func() {
    printf("Hello world!\n");
    setcontext(&context_sleep);
}

void timer_handler(int signum) {
    swapcontext(&context_sleep, &context_echo);
}

int main() {
    // Set up signal handler for timer
    struct sigaction sa;
    sa.sa_handler = timer_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, NULL);
    
    // Set up timer
    struct itimerval timer;
    timer.it_value.tv_sec = QUANTUM;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = QUANTUM;
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);

    // Set up contexts
    char stack_sleep[STACK_SIZE];
    char stack_echo[STACK_SIZE];
    getcontext(&context_sleep);
    context_sleep.uc_stack.ss_sp = stack_sleep;
    context_sleep.uc_stack.ss_size = sizeof(stack_sleep);
    context_sleep.uc_link = &context_echo;
    makecontext(&context_sleep, sleep_func, 0);
    getcontext(&context_echo);
    context_echo.uc_stack.ss_sp = stack_echo;
    context_echo.uc_stack.ss_size = sizeof(stack_echo);
    context_echo.uc_link = &context_sleep;
    makecontext(&context_echo, echo_func, 0);

    // Start first process
    setcontext(&context_sleep);

    return 0;
}
