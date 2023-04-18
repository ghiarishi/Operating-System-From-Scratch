#pragma once

#include "fat.h"
#include "../process/dependencies.h"

#define PSTDIN_FILENO 0
#define PSTDOUT_FILENO 1

extern fs_t *fs;
extern Process *activeProcess;

int f_open(const char *fname, int mode);
int f_close(int fd);
ssize_t f_read(int fd, int n, char *buf);
ssize_t f_write(int fd, const char *str, ssize_t n);
int f_unlink(const char *fname);
uint32_t f_lseek(int fd, int offset, int whence);
filestat_t **f_ls(const char *fname);
void f_freels(filestat_t **stat);
int f_rename(const char *oldname, const char *newname);
int f_chmod(int fd, char mode, uint8_t bitset);
