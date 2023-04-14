#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <ucontext.h>
#include "scheduler.h"
#include "dependencies.h"

void echoFunc(int argc, char *argv[]);
void sleepFunc(char *argv[], int ticksLeft);
void busy_wait(void);
// void lsFunc();
// void touchFunc(const char* filename);