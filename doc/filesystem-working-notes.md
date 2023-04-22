# Filesystem Working Notes

Contributors: Andrew Zhu <!-- add yourself here -->

File layout:

- src/
  - pennfat.c - entrypoint for the pennfat standalone
  - fs/
    - fat.c/.h - kernel-level fat functions, file struct defs
    - user.c/.h - user-level fat functions

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

#define PETOOMANYF 1101 // you have too many files open already
#define PESTDIO 1102    // tried to read from stdout or write to stdin
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

## File Locking Mechanism

In order to prevent multiple writers/conflicting read-writes, the PennFAT filesystem grants exclusive access to any
process that opens a file in a writing mode, and shared access to processes opening a file in read mode. To accomplish
this, the filesystem keeps a record of what files have been opened and in what mode; if a call to `fs_open()` would
violate the locking semantics, the syscall fails with an error.

This record is a `file_t *` array that utilizes array doubling to grow dynamically (initially sized at 4).

## Standalone

The standalone completes the demo plan with no "definitely/indirectly/possibly lost" memory leaks.

## Syscalls

### int f_open(const char *fname, int mode)

Open a file. If the file is opened in F_WRITE or F_APPEND mode, the file is created if it does not exist.

**Parameters**
- `name`: the name of the file to open
- `mode`: the mode to open the file in (F_WRITE, F_READ, or F_APPEND).

**Returns**
the file descriptor of the opened file, -1 on error

**Exceptions**
- `PENOFILE`: the requested file was in read mode and does not exist
- `PEHOSTIO`: failed to read from/write to host filesystem
- `PETOOFAT`: the operation would make a new file but the filesystem is full
- `PEFNAME`: the operation would make a new file but the filename is invalid
- `PEINUSE`: the requested file was opened in an exclusive mode and is currently in use

### int f_close(int fd)

Closes the specified file, freeing any associated memory.

**Parameters**
- `fd`: the file to close

**Returns**
0 on a success, -1 on error.

**Exceptions**
- `PEINVAL`: the file descriptor is invalid

### ssize_t f_read(int fd, int n, char *buf)

Read up to `n` bytes from the specified file into `buf`.

**Parameters**
- `fd`: the file to read from
- `n`: the maximum number of bytes to read
- `buf`: a buffer to store the read bytes

**Returns**
the number of bytes read; -1 on error

**Exceptions**
- `PEFPERM`: you do not have permission to read this file
- `PEHOSTIO`: failed to read from host filesystem

### ssize_t f_write(int fd, const char *str, ssize_t n)

Write up to `n` bytes from `buf` into the specified file.

**Parameters**
- `fd`: the file to write to
- `str`: a buffer storing the bytes to write
- `b`: the maximum number of bytes to write

**Returns**
the number of bytes written; -1 on error

**Exceptions**
- `PEFMODE`: the file is not in write or append mode
- `PEFPERM`: you do not have permission to write to this file
- `PEHOSTIO`: failed to read from host filesystem
- `PETOOFAT`: filesystem is full

### int f_unlink(const char *fname)

Removes the file with the given name.

**Parameters**
- `fname`: the name of the file to delete

**Returns**
0 on success; -1 on error

**Exceptions**
- `PENOFILE`: the specified file does not exist
- `PEHOSTIO`: failed to i/o with the host entry

### uint32_t f_lseek(int fd, int offset, int whence)

Seek the file offset to the given position, given by `offset`. If `whence` is `F_SEEK_SET` this is relative to the 
start of the file, for `F_SEEK_CUR` relative to the current position, and for `F_SEEK_END` relative to the end of the 
file.

**Parameters**
- `fd`: the file to seek
- `offset`: where to seek to relative to `whence`
- `whence`: the seek mode

**Returns**
the new location in bytes from start of file; -1 on error

**Exceptions**
- `PEINVAL`: whence is not a valid option

### filestat_t **f_ls(const char *fname)

Gets information for a file. If `fname` is NULL, gets information for all the files.
It is the caller's responsibility to free each of the returned structs. Use the convenience function `f_freels()` to
do this quickly.

**Parameters**
- `fname`: the name of the file to get the stat of, or NULL to list all files

**Returns**
a pointer to an array of filestat struct pointers. The array will always be terminated with a NULL pointer.

**Exceptions**
- `PEHOSTIO`: failed to read from host filesystem

### void f_freels(filestat_t **stat)

Free the filestat list returned by `f_ls()`.

### int f_rename(const char *oldname, const char *newname)

Rename a file.

**Parameters**
- `oldname`: the old name
- `newname`: the new name

**Returns**
0 on success, -1 on failure

**Exceptions**
- `PENOFILE`: the file does not exist
- `PEHOSTIO`: failed to perform IO on host drive
- `PEFNAME`: the new name is invalid

### int f_chmod(int fd, char mode, uint8_t bitset)

Edit the I/O permissions of an open file.

**Parameters**
- `fd`: the file whose permissions to edit
- `mode`: the mode to edit it in; '+', '=', or '-'
- `bitset`: the permission bitset to edit by (a combination of FAT_EXECUTE, FAT_WRITE, and FAT_READ)

**Returns**
0 on success, -1 on error

**Exceptions**
- `PEINVAL`: the mode is invalid