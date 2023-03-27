#pragma once

#include "fat.h"
#include "../process/scheduler.h"
#include "../process/pcb.h"

pid_t p_spawn(void (*func)(), char *argv[], int fd0, int fd);

// #define PSTDIN_FILENO 0
// #define PSTDOUT_FILENO 1
