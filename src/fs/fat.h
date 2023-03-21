#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "../common/errno.h"

// fs_open()
#define F_WRITE 1
#define F_READ 2
#define F_APPEND 3

// fs_lseek()
#define F_SEEK_SET 1
#define F_SEEK_CUR 2
#define F_SEEK_END 3

// FAT internals
#define FAT_EOF 0xFFFF
#define FAT_NAME_LEN 32
#define FAT_FILE_SIZE 64

typedef struct filesystem {
    uint32_t fat_size;  // size, in bytes, of the FAT
    uint16_t block_size;  // size, in bytes, of each block
    uint16_t *fat;  // ptr to uint16_t[fat_size / 2]; entry 0 used for table/block size
    // data: uint8_t[block_size * (n_entries - 1)]; 0-indexed
    int host_fd;
} fs_t;


typedef struct file {
    // filesystem internals
    char name[FAT_NAME_LEN];
    uint32_t size;
    uint16_t blockno;
    uint8_t type;
    uint8_t perm;
    time_t mtime;
    uint8_t unused[16];

    // os internals
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

// low-level helpers
file_t *fs_makefile(fs_t *fs, const char *name, int mode);
ssize_t fs_read_blk(fs_t *fs, uint16_t blk_base_no, uint32_t offset, uint32_t len, void *buf);
ssize_t fs_write_blk(fs_t *fs, uint16_t blk_base_no, uint32_t offset, void *str, uint32_t len);
