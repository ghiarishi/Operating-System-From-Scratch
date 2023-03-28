#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ucontext.h>
#include "scheduler.h"

void echoFunc(int argc, char *argv[]);

void sleepFunc(int milliseconds);