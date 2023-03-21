#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "../common/errno.h"

// fs_open()
#define F_WRITE 1
#define F_READ 2
#define F_APPEND 3

// fs_lseek()
#define F_SEEK_SET 1
#define F_SEEK_CUR 2
#define F_SEEK_END 3

typedef struct filesystem {
    uint32_t fat_size;  // size, in bytes, of the FAT
    uint16_t block_size;  // size, in bytes, of each block
    uint16_t *fat;  // ptr to uint16_t[fat_size / 2]; entry 0 used for table/block size
    // data: uint8_t[block_size * (n_entries - 1)]; 0-indexed
    int host_fd;
} fs_t;


typedef struct file {
    char name[32];
    uint32_t size;
    uint16_t blockno;
    uint8_t type;
    uint8_t perm;
    time_t mtime;
    uint32_t offset;  // current seek position
    int mode;
} file_t;

fs_t *fs_mount(const char *path);
int fs_unmount(fs_t *fs);
file_t *fs_open(fs_t *fs, const char *name, int mode);
int fs_close(fs_t *fs, file_t *f);
int fs_read(fs_t *fs, file_t *f, uint32_t len, char *buf);
int fs_write(fs_t *fs, file_t *f, const char *str, uint32_t len);
int fs_lseek(fs_t *fs, file_t *f, int offset, int whence);
int fs_unlink(fs_t *fs, const char *fname);
int fs_ls(fs_t *fs, const char *fname);
