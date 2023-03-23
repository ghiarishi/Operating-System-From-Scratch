#include "fat.h"

/**
 * Mount the PennFAT filesystem found at the path.
 * @param path path to the filesystem file on the host os to read
 * @param fs pointer to store the resulting file system struct
 * @return a pointer to the newly mounted filesystem; NULL on failure (sets *ERRNO*)
 * @throw PEHOSTFS Could not open the specified file
 * @throw PEHOSTIO Could not read or mmap from the host file
 * @throw PEBADFS The filesystem file is invalid
 */
fs_t *fs_mount(const char *path) {
    fs_t *fs = malloc(sizeof(fs_t));

    // open host system file
    int fd = open(path, O_RDWR);
    if (fd == -1)
        raise_n(PEHOSTFS);
    fs->host_fd = fd;

    // read the first 2 bytes for blockno info and block size
    uint8_t info[2];
    ssize_t bytes_read;
    bytes_read = read(fd, info, 2);
    if (bytes_read == -1)
        raise_n(PEHOSTIO);
    else if (bytes_read < 2)
        raise_n(PEBADFS);

    // ensure that the filesystem is valid and load in filesystem info
    if (info[0] == 0 || info[0] > 32)
        raise_n(PEBADFS);
    switch (info[1]) {
        case 0:
            fs->block_size = 256;
            break;
        case 1:
            fs->block_size = 512;
            break;
        case 2:
            fs->block_size = 1024;
            break;
        case 3:
            fs->block_size = 2048;
            break;
        case 4:
            fs->block_size = 4096;
            break;
        default:
            raise_n(PEBADFS);
    }
    fs->fat_size = fs->block_size * info[0];

    // load in the FAT into memory
    if (lseek(fd, 0, SEEK_SET) == -1)
        raise_n(PEHOSTIO);
    fs->fat = mmap(NULL, fs->fat_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fs->fat == MAP_FAILED)
        raise_n(PEHOSTIO);

    // and we're good
    return fs;
}

/**
 * Unmount a filesystem and free any associated memory.
 * @param fs the fs to free
 * @return 0 normally; -1 on error
 * @throw PEHOSTFS could not close the host file for the fs
 */
int fs_unmount(fs_t *fs) {
    // unlink the memory and free the heap-alloced fs
    munmap(fs->fat, fs->fat_size);
    if (close(fs->host_fd) == -1) raise(PEHOSTFS);
    free(fs);
    return 0;
}

/**
 * Open a file in the filesystem. If the file is opened in F_WRITE or F_APPEND mode, the file is created if it does not
 * exist.
 * TODO: A file can be opened any number of times in F_READ mode but only once at a time in F_WRITE or F_APPEND mode. // maybe this should be at the OS level so fs doesn't have to track open files
 * If the provided file name is longer than 31 characters and a new file is created, the created file's name will be
 * truncated to 31 characters.
 * @param fs the filesystem
 * @param name the name of the file to open
 * @param mode the mode to open the file in (F_WRITE, F_READ, or F_APPEND).
 * @return the opened file, NULL on error
 * @throw PENOFILE the requested file was in read mode and does not exist
 * @throw PEHOSTIO failed to read from/write to host filesystem
 * @throw PETOOFAT the operation would make a new file but the filesystem is full
 * @throw PEINVAL the operation would make a new file but the filename is invalid
 */
file_t *fs_open(fs_t *fs, const char *name, int mode) {
    uint32_t offset = fs_find(fs, name);

    // if the file was not found, create it if in a creation mode
    if (offset == -1) {
        if (mode == F_READ) raise_n(PENOFILE);
        return fs_makefile(fs, name, mode);
    }

    // file was found
    ssize_t r;
    file_t *f = malloc(sizeof(file_t));
    r = fs_read_blk(fs, 1, offset, FAT_FILE_SIZE, f);
    if (r == -1) return NULL;
    if (r != FAT_FILE_SIZE) raise_n(PEHOSTIO);
    f->offset = mode == F_APPEND ? f->size : 0;
    f->mode = mode;

    // if the file was found and we are in write mode, truncate it to 0 bytes
    if (mode == F_WRITE) {
        // traverse FAT and set all blocks past first to free
        if (fs->fat[f->blockno] != FAT_EOF) {
            uint16_t blockno = fs->fat[f->blockno];
            fs->fat[f->blockno] = FAT_EOF;
            do {
                uint16_t next_block = fs->fat[blockno];
                fs->fat[blockno] = FAT_FREE;
                blockno = next_block;
            } while (fs->fat[blockno] != FAT_EOF);
        }
        f->size = 0;
    }
    return f;
}

/**
 * Closes the specified file, freeing any associated memory.
 * @param fs the filesystem
 * @param f the file to close
 * @return 0 on a success, -1 on error.
 */
int fs_close(fs_t *fs, file_t *f) {
    free(f);
    return 0;
}

/**
 * Read up to `n` bytes from the specified file into `buf`.
 * @param fs the filesystem to read from
 * @param f the file to read from
 * @param len the maximum number of bytes to read
 * @param buf a buffer to store the read bytes
 * @return the number of bytes read; -1 on error
 * @throw PEFPERM you do not have permission to read this file
 * @throw PEHOSTIO failed to read from host filesystem
 */
ssize_t fs_read(fs_t *fs, file_t *f, uint32_t len, char *buf) {
    if (!(f->perm & FAT_READ)) raise(PEFPERM);
    ssize_t bytes_read = fs_read_blk(fs, f->blockno, f->offset, len, buf);
    f->offset += bytes_read;
    return bytes_read;
}

/**
 * Write up to `n` bytes from `buf` into the specified file.
 * @param fs the filesystem to write to
 * @param f the file to write to
 * @param str a buffer storing the bytes to write
 * @param len the maximum number of bytes to write
 * @return the number of bytes written; -1 on error
 * @throw PEFMODE the file is not in write or append mode
 * @throw PEFPERM you do not have permission to write to this file
 * @throw PEHOSTIO failed to read from host filesystem
 * @throw PETOOFAT filesystem is full
 */
ssize_t fs_write(fs_t *fs, file_t *f, const char *str, uint32_t len) {
    if (!(f->mode == F_WRITE || f->mode == F_APPEND)) raise(PEFMODE);
    if (!(f->perm & FAT_WRITE)) raise(PEFPERM);
    // append mode always seeks to EOF before writing
    if (f->mode == F_APPEND) fs_lseek(fs, f, 0, F_SEEK_END);

    // if the offset is past EOF, fill in hole with null bytes
    while (f->offset > f->size) {
        uint8_t zeroes[fs->block_size];
        bzero(zeroes, fs->block_size);
        uint32_t to_write = f->offset - f->size < fs->block_size ? f->offset - f->size : fs->block_size;
        ssize_t w = fs_write_blk(fs, f->blockno, f->size, zeroes, to_write);
        if (w == -1) return -1;
        f->size += w;
    }

    // write to file
    ssize_t bytes_written = fs_write_blk(fs, f->blockno, f->offset, str, len);
    f->offset += bytes_written;
    if (f->offset > f->size) {
        f->size = f->offset;
    }
    return bytes_written;
}

/**
 * Seek the file offset to the given position, given by `offset`. If `whence` is `F_SEEK_SET` this is relative to the
 * start of the file, for `F_SEEK_CUR` relative to the current position, and for `F_SEEK_END` relative to the end of
 * the file.
 * @param fs the filesystem
 * @param f the file to seek
 * @param offset where to seek to relative to `whence`
 * @param whence the seek mode
 * @return the new location in bytes from start of file; -1 on error
 * @throw PEINVAL whence is not a valid option
 */
uint32_t fs_lseek(fs_t *fs, file_t *f, int offset, int whence) {
    switch (whence) {
        case F_SEEK_SET:
            f->offset = offset;
            break;
        case F_SEEK_CUR:
            f->offset += offset;
            break;
        case F_SEEK_END:
            f->offset = f->size + offset;
            break;
        default:
            raise(PEINVAL);
    }
    return f->offset;
}

/**
 * Removes the file with the given name. Frees any associated memory in the FAT.
 * @param fs the filesystem
 * @param fname the name of the file to delete
 * @return 0 on success; -1 on error
 * @throw PENOFILE the specified file does not exist
 * @throw PEHOSTIO failed to i/o with the host fs
 */
int fs_unlink(fs_t *fs, const char *fname) {
    uint32_t offset = fs_find(fs, fname);
    if (offset == -1) raise(PENOFILE);
    // get fileinfo to free all the alloc'd memory
    filestat_t f;
    if (fs_read_blk(fs, 1, offset, FAT_FILE_SIZE, &f) != FAT_FILE_SIZE) raise(PEHOSTIO);
    uint16_t blockno = f.blockno;
    do {
        uint16_t next_blockno = fs->fat[blockno];
        fs->fat[blockno] = FAT_FREE;
        blockno = next_blockno;
    } while (blockno != FAT_EOF);
    // then mark the file as deleted in the root dir
    // todo is this file still in use?
    // todo if so, set this to 2
    // todo otherwise, set it to 1
    uint8_t one = 1;
    if (fs_write_blk(fs, 1, offset, &one, 1) != 1) raise(PEHOSTIO);
    return 0;
}

/**
 * Gets information for a file. If `fname` is NULL, gets information for all the files.
 * It is the caller's responsibility to free each of the returned structs. Use the convenience method `fs_freels()` to
 * do this quickly.
 * @param fs the filesystem
 * @param fname the name of the file to get the stat of, or NULL to list all files
 * @return a pointer to a list of filestat struct pointers. The array will always be terminated with a NULL pointer.
 * @throw PEHOSTIO failed to read from host filesystem
 */
filestat_t **fs_ls(fs_t *fs, const char *fname) {
    if (fname == NULL) return fs_lsall(fs);
    // return stat for one file, as array of 1 ptr
    uint32_t offset = fs_find(fs, fname);
    if (offset == -1) return NULL;
    filestat_t **fstat = malloc(sizeof(filestat_t *) * 2);
    fstat[0] = malloc(sizeof(filestat_t));
    ssize_t r = fs_read_blk(fs, 1, offset, FAT_FILE_SIZE, fstat[0]);
    if (r != FAT_FILE_SIZE) {
        free(fstat[0]);
        free(fstat);
        ERRNO = PEHOSTIO;
        return NULL;
    }
    fstat[1] = NULL;
    return fstat;
}

/**
 * Helper for fs_ls.
 */
filestat_t **fs_lsall(fs_t *fs) {
    // traverse the root directory, use array doubling starting at n = 4
    int size = 4, count = 0;
    filestat_t **f = malloc(sizeof(filestat_t *) * size);
    uint32_t offset = 0;
    filestat_t tempf;
    ssize_t r;
    do {
        r = fs_read_blk(fs, 1, offset, FAT_FILE_SIZE, &tempf);
        if (r == -1) {
            // error; free anything we've alloced
            for (int i = 0; i < count; ++i) {
                free(f[i]);
            }
            free(f);
            return NULL;
        }
        if (tempf.name[0] == 0) return f; // end of directory
        if (tempf.name[0] > 2) {
            // copy to end of array
            filestat_t *newf = malloc(sizeof(filestat_t));
            memcpy(newf, &tempf, sizeof(filestat_t));
            f[count] = newf;
            count++;
            // if the array needs more space, double it
            if (count == size) {
                size *= 2;
                f = realloc(f, sizeof(filestat_t *) * size);
            }
        }
        offset += FAT_FILE_SIZE;
    } while (r);
    f[count] = NULL;
    return f;
}

// ==== low-level helpers ====
/**
 * Creates a new file with the given name in the next open block.
 * @param fs the filesystem
 * @param name the name of the file
 * @param mode the mode to return the newly-created file in
 * @return the new file, NULL on error
 * @throw PEINVAL file name is invalid
 * @throw PEHOSTIO failed to read from/write to host filesystem
 * @throw PETOOFAT the filesystem is full
 */
file_t *fs_makefile(fs_t *fs, const char *name, int mode) {
    // validate filename; must be at least 1 character and alnum/[._-]
    int namelen = strlen(name); // NOLINT(cppcoreguidelines-narrowing-conversions)
    char letter;
    if (!namelen) raise_n(PEINVAL);
    for (int i = 0; i < namelen; ++i) {
        letter = name[i];
        if (!(isalnum(letter) || letter == '-' || letter == '_' || letter == '.')) raise_n(PEINVAL);
    }

    // traverse the root directory looking for deleted files or end of dir
    uint32_t offset = 0;
    char indicator;
    ssize_t r;
    while (1) {
        r = fs_read_blk(fs, 1, offset, 1, &indicator);
        if (r == -1) return NULL;
        if (indicator == 0 || indicator == 1 || r == 0) break; // end of directory or deleted file
        offset += FAT_FILE_SIZE;
    }

    // if we stopped searching because we hit the end of the directory block, write 0s to the next block
    if (r == 0) {
        uint8_t zeroes[fs->block_size];
        bzero(zeroes, fs->block_size);
        r = fs_write_blk(fs, 1, offset, zeroes, fs->block_size);
        if (r == -1) return NULL;
        if (r != fs->block_size) raise_n(PEHOSTIO);
    }

    // find the next free block
    uint16_t block = fs_link_next_free(fs);
    if (!block) raise_n(PETOOFAT);

    // here is where I will make my file
    // create the file object
    file_t *f = malloc(sizeof(file_t));
    strncpy(f->name, name, FAT_NAME_LEN - 1);
    f->name[FAT_NAME_LEN - 1] = 0;
    f->blockno = block;
    f->type = 1;  // regular file
    f->perm = FAT_READ | FAT_WRITE;  // read/write
    time(&f->mtime);
    f->offset = 0;
    f->mode = mode;
    // write to the directory
    r = fs_write_blk(fs, 1, offset, f, FAT_FILE_SIZE);
    if (r == -1) return NULL;
    if (r != FAT_FILE_SIZE) raise_n(PEHOSTIO);
    return f;
}

/**
 * Similar to fs_read, except takes a block number and offset. Traverses the FAT if offset > fs.block_size.
 * @param fs the filesystem to read from
 * @param blk_base_no the block number the file starts in
 * @param offset where to start reading relative to the base block number
 * @param len the maximum number of bytes to read
 * @param buf a buffer to store the read bytes
 * @return the number of bytes read; -1 on error
 * @throw PEINVAL the provided block number or offset is outside the filesystem
 * @throw PEHOSTIO failed to read from host filesystem
 */
ssize_t fs_read_blk(fs_t *fs, uint16_t blk_base_no, uint32_t offset, uint32_t len, void *buf) {
    if (blk_base_no >= fs->fat_size / 2 || blk_base_no == 0) raise(PEINVAL);
    // seek to the right offset
    while (offset >= fs->block_size) {
        uint16_t next_block = fs->fat[blk_base_no];
        if (next_block == FAT_EOF) return 0;
        offset -= fs->block_size;
        blk_base_no = next_block;
    }
    // seek to the right spot on host
    if (lseek(fs->host_fd, fs->fat_size + fs->block_size * (blk_base_no - 1) + offset, SEEK_SET) == -1) raise(PEHOSTIO);
    // do we need to split the read?
    ssize_t bytes_read, r;
    if (offset + len > fs->block_size) {
        // read as much as we can from this block, then recursive call to read from the next
        bytes_read = read(fs->host_fd, buf, fs->block_size - offset);
        if (bytes_read == -1) raise(PEHOSTIO);
        uint16_t next_block = fs->fat[blk_base_no];
        if (next_block == FAT_EOF) return bytes_read;
        r = fs_read_blk(fs, next_block, 0, len - bytes_read, buf + bytes_read);
        if (r == -1) return -1;
        return bytes_read + r;
    } else {
        // read the requested length from the block
        bytes_read = read(fs->host_fd, buf, len);
        if (bytes_read == -1) raise(PEHOSTIO);
        return bytes_read;
    }
}

/**
 * Similar to fs_read, except takes a block number and offset. Traverses the FAT if offset > fs.block_size.
 * If the write would overflow past the EOF block, allocs a new block.
 * @param fs the filesystem to read from
 * @param blk_base_no the block number the file starts in
 * @param offset where to start reading relative to the base block number
 * @param len the maximum number of bytes to write
 * @param buf a buffer to store the bytes to write
 * @return the number of bytes written; -1 on error
 * @throw PEINVAL the provided block number or offset is outside the filesystem
 * @throw PEHOSTIO failed to read from host filesystem
 * @throw PETOOFAT filesystem is full
 */
ssize_t fs_write_blk(fs_t *fs, uint16_t blk_base_no, uint32_t offset, const void *str, uint32_t len) {
    if (blk_base_no >= fs->fat_size / 2 || blk_base_no == 0) raise(PEINVAL);
    // seek to the right offset
    while (offset >= fs->block_size) {
        uint16_t next_block = fs->fat[blk_base_no];
        if (next_block == FAT_EOF) {
            // alloc a new block and link it
            next_block = fs_link_next_free(fs);
            if (!next_block) raise(PETOOFAT);
            fs->fat[blk_base_no] = next_block;
        }
        offset -= fs->block_size;
        blk_base_no = next_block;
    }
    // seek to the right spot on host
    if (lseek(fs->host_fd, fs->fat_size + fs->block_size * (blk_base_no - 1) + offset, SEEK_SET) == -1) raise(PEHOSTIO);
    // do we need to split the write?
    ssize_t bytes_written, r;
    if (offset + len > fs->block_size) {
        // write as much as we can to this block, then recursive call to write to the next
        bytes_written = write(fs->host_fd, str, fs->block_size - offset);
        if (bytes_written == -1) raise(PEHOSTIO);
        uint16_t next_block = fs->fat[blk_base_no];
        if (next_block == FAT_EOF) {
            // alloc a new block and link it
            next_block = fs_link_next_free(fs);
            if (!next_block) raise(PETOOFAT);
            fs->fat[blk_base_no] = next_block;
        }
        r = fs_write_blk(fs, next_block, 0, str + bytes_written, len - bytes_written);
        if (r == -1) return -1;
        return bytes_written + r;
    } else {
        // read the requested length from the block
        bytes_written = write(fs->host_fd, str, len);
        if (bytes_written == -1) raise(PEHOSTIO);
        return bytes_written;
    }
}

/**
 * Finds the next free block on the filesystem and marks it as used in the FAT.
 * @param fs the filesystem
 * @return The next free block number (1-indexed), or 0 if there are no free blocks.
 */
uint16_t fs_link_next_free(fs_t *fs) {
    uint16_t block = 1;
    while (block < fs->fat_size / 2) {
        if (fs->fat[block] == 0) {
            fs->fat[block] = FAT_EOF;
            return block;
        }
        block++;
    }
    return 0;
}

/**
 * Find the offset of the file with the given name in the root directory.
 * @param fs the filesystem
 * @param fname the name of the file to search for
 * @return the offset of the file entry from the root dir on success, -1 (0xffffffff) on error
 * @throw PEHOSTIO failed to read from host filesystem
 */
uint32_t fs_find(fs_t *fs, const char *fname) {
    // traverse the root directory looking for the file with given name
    uint32_t offset = 0;
    char tempname[FAT_NAME_LEN];
    ssize_t r;
    do {
        r = fs_read_blk(fs, 1, offset, FAT_NAME_LEN, tempname);
        if (r == -1) return -1;
        if (tempname[0] == 0) return -1; // end of directory
        if (strncmp(tempname, fname, FAT_NAME_LEN) == 0) {
            return offset;
        };
        offset += FAT_FILE_SIZE;
    } while (r);
    return -1;
}

/**
 * Free the filestat list returned by `fs_ls()`.
 */
void fs_freels(filestat_t **stat) {
    for (int i = 0; stat[i] != NULL; ++i) {
        free(stat[i]);
    }
    free(stat);
}