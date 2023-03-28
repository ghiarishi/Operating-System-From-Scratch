#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext
#include "pcb.h"
#include "scheduler.h"


struct Process* createNewProcess(void (*func)(), char* argv[], int id, int priority);