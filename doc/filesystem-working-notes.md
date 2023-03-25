# Filesystem Working Notes

Contributors: Andrew Zhu <!-- add yourself here -->

## Namespacing

All kernel-level functions will start with the prefix `fs_`.

All user-level functions will start with the prefix `f_`.

## Error Codes

PennOS specific filesystem errors will be in the 1000-2000 range.

```c
#define PEHOSTFS 1001  // could not open/close file in host filesystem
#define PEHOSTIO 1002  // could not perform I/O in host filesystem
#define PEBADFS 1003   // invalid PennFAT file, or was otherwise unable to mount
#define PENOFILE 1004  // specified file does not exist
#define PEINUSE 1005   // the specified file is in use in another context and an exclusive operation was called
#define PETOOFAT 1006  // the filesystem is too fat and has no space for a new file
#define PEFMODE 1007   // attempted operation on a file in the wrong mode
#define PEFPERM 1008   // attempted operation on a file without read/write permissions
#define PEFNAME 1009   // the filename is invalid
```

## Structs

When a file is opened, it returns a `struct file`:

```c
typedef struct file {
    filestat_t *entry;  // mmaped to file entry in directory
    uint32_t offset;  // current seek position
    int mode;
    uint8_t stdiomode;  // 0 = FAT file, 1 = stdout, 2 = stdin
} file_t;
```

where `filestat_t` is the directory entry defined in the PennOS handout:

```c
typedef struct filestat {
    char name[32];
    uint32_t size;
    uint16_t blockno;  // will be unique for each file
    uint8_t type;
    uint8_t perm;
    time_t mtime;
    uint8_t unused[16];
} filestat_t;
```

The OS should maintain a file descriptor table for each process linking an int to one of these `struct file`s.
The low-level filesystem implementation operates using pointers to open file structs.

## stdin/stdout

Each `file_t *` struct contains information on whether it is a special file for reading from/writing to stdin/stdout.
The user-level functions `f_read()` and `f_write()` should check this information and redirect to the C API where
necessary rather than the FAT API.

Each process, on creation, should set entries `PSTDIN_FILENO` and `PSTDOUT_FILENO` in its PCB's fd table to file structs
with the correct flag set. This allows for later redirecting stdin/stdout by overwriting the entries in the fd table
with file structs linked to files on the FAT filesystem.
