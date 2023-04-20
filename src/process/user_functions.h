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
void sleepFunc(int argc, char *argv[]);
void busy_wait(void);
void idleFunc();

// ==== filesystem ====
void catFunc(int argc, char **argv);
void lsFunc(int argc, char **argv);
void touchFunc(int argc, char **argv);
void mvFunc(int argc, char **argv);
void cpFunc(int argc, char **argv);
void rmFunc(int argc, char **argv);
void chmodFunc(int argc, char **argv);

void psFunc (int argc, char **argv);

void zombify(int argc, char **argv);
void zombie_child();
void orphan_child();
void orphanify(int argc, char **argv);
void logout(int argc, char **argv);
