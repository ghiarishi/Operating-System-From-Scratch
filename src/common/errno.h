#pragma once

// errno global
int ERRNO = 0;

// helpers to set errno and return special codes (-1, NULL, etc)
#define raise(code) do {ERRNO = code; return -1;} while(0)
#define raise_n(code) do {ERRNO = code; return NULL;} while(0)

// ==== generic ====
#define EINVAL 22  // generic invalid argument given to syscall

// ==== filesystem ====
// error space: 1000-2000
#define EHOSTFS 1001  // could not operate on file in host filesystem
#define EHOSTIO 1002  // could not perform I/O in host filesystem
#define EBADFS 1003   // invalid PennFAT file, or was otherwise unable to mount
#define
