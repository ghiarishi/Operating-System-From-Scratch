#pragma once

#include <stdio.h>  // for fprintf() to stderr

// errno global
extern int ERRNO;

// helpers to set errno and return special codes (-1, NULL, etc)
#define raise(code) do {ERRNO = code; return -1;} while(0)
#define raise_n(code) do {ERRNO = code; return NULL;} while(0)

// ==== error codes ====
// prefixed with PE to differentiate from E in <errno.h>
// each of these should be added to p_perror() after defining here
// ==== generic ====
#define PEINVAL 22  // generic invalid argument given to syscall

// ==== filesystem ====
// error space: 1000-2000
#define PEHOSTFS 1001  // could not open/close file in host filesystem
#define PEHOSTIO 1002  // could not perform I/O in host filesystem
#define PEBADFS 1003   // invalid PennFAT file, or was otherwise unable to mount
#define PENOFILE 1004  // specified file does not exist
#define PEINUSE 1005   // the specified file is in use in another context and an exclusive operation was called
#define PETOOFAT 1006  // the filesystem is too fat and has no space for a new file
#define PEFMODE 1007   // attempted operation on a file in the wrong mode
#define PEFPERM 1008   // attempted operation on a file without read/write permissions
#define PEFNAME 1009   // the filename is invalid

#define PETOOMANYF 1101 // you have too many files open already

// ==== functions ====
void p_perror(const char *prefix);
