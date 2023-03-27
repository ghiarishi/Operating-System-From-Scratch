#pragma once

#include "scheduler.h"
#include "pcb.h"
#include <ucontext.h>

//void testFunc1()
pid_t p_spawn(void (*func)(), char *argv[], int fd0, int fd);
