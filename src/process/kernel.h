#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext
#include "pcb.h"
#include "scheduler.h"

struct pcb* k_process_create(struct pcb *parent, void (*func)(), char* argv[], int id, int priority);