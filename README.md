# PennOS

## errno.h

All PennOS error codes are listed in `src/common/errno.h`. When adding a new error code, here are the steps you should
follow:

1. Define the new error code in `src/common/errno.h`. I recommend "scoping" the error numbers (e.g. filesystem errors in
   the range 1000-2000, kernel in the range 2000-3000, etc.)
2. Add the message for the error code to `p_perror()` in `src/common/errno.c`.

## Contributions

### Andrew Zhu (andrz)

- Makefile
- basic CI
- all filesystem kernel-level methods
- low-level FAT implementation
- pennfat standalone (all commands)
- main errno implementation
