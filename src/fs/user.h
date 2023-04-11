#pragma once

#include "fat.h"

#define PSTDIN_FILENO 0
#define PSTDOUT_FILENO 1

int f_open(const char *fname, int mode);
int f_close(int fd);
int f_read(int fd, int n, char *buf);
int f_write(int fd, const char *str, int n);