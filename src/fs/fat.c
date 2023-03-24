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
    // open host system file
    int fd = open(path, O_RDWR);
    if (fd == -1)
        raise_n(PEHOSTFS);

    // read the first 2 bytes for blockno info and block size
    uint8_t info[2];
    ssize_t bytes_read;
    bytes_read = read(fd, info, 2);
    if (bytes_read == -1)
        raise_n(PEHOSTIO);
    else if (bytes_read < 2)
        raise_n(PEBADFS);

    // ensure that the filesystem is valid and load in filesystem info
    uint16_t block_size;
    if (info[1] == 0 || info[1] > 32)
        raise_n(PEBADFS);
    switch (info[0]) {
        case 0:
            block_size = 256;
            break;
        case 1:
            block_size = 512;
            break;
        case 2:
            block_size = 1024;
            break;
        case 3:
            block_size = 2048;
            break;
        case 4:
            block_size = 4096;
            break;
        default:
            raise_n(PEBADFS);
    }

    // calculations
    int fat_blocks = info[1];
    int n_entries = block_size * fat_blocks / 2;
    int data_size = block_size * (n_entries - 1);
    // special case: if n_entries = 0x10000, cut off the end block (0xffff) since we can't reach it
    if (n_entries == 0x10000) data_size -= block_size;

    // load in the FAT into memory
    if (lseek(fd, 0, SEEK_SET) == -1)
        raise_n(PEHOSTIO);

    // create the fs struct and map the FAT, and whole data block
    fs_t *fs = malloc(sizeof(fs_t));
    fs->host_fd = fd;
    fs->fat_size = block_size * fat_blocks;
    fs->block_size = block_size;
    fs->fat = mmap(NULL, fs->fat_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fs->fat == MAP_FAILED) {
        free(fs);
        raise_n(PEHOSTIO);
    }
    fs->data = mmap(NULL, fs->fat_size + data_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fs->fat == MAP_FAILED) {
        free(fs);
        raise_n(PEHOSTIO);
    }
    fs->opened_size = 4;
    fs->opened_count = 0;
    fs->open_files = malloc(sizeof(file_t *) * fs->opened_size);

    // and we're good
    return fs;
}

/**
 * Unmount a filesystem and free any associated memory.
 * Any open files in the filesystem should be closed first to avoid memory leaks.
 * @param fs the entry to free
 * @return 0 normally; -1 on error
 * @throw PEHOSTFS could not close the host file for the entry
 */
int fs_unmount(fs_t *fs) {
    // unlink the memory and free the heap-alloced entry
    munmap(fs->fat, fs->fat_size);
    if (close(fs->host_fd) == -1) raise(PEHOSTFS);
    free(fs->open_files);
    free(fs);
    return 0;
}

/**
 * Open a file in the filesystem. If the file is opened in F_WRITE or F_APPEND mode, the file is created if it does not
 * exist.
 * @param fs the filesystem
 * @param name the name of the file to open
 * @param mode the mode to open the file in (F_WRITE, F_READ, or F_APPEND).
 * @return the opened file, NULL on error
 * @throw PENOFILE the requested file was in read mode and does not exist
 * @throw PEHOSTIO failed to read from/write to host filesystem
 * @throw PETOOFAT the operation would make a new file but the filesystem is full
 * @throw PEFNAME the operation would make a new file but the filename is invalid
 * @throw PEINUSE the requested file was opened in an exclusive mode and is currently in use
 */
file_t *fs_open(fs_t *fs, const char *name, int mode) {
    uint32_t offset = fs_find(fs, name);

    // if the file was not found, create it if in a creation mode
    if (offset == -1) {
        if (mode != F_READ && ERRNO == PENOFILE) return fs_makefile(fs, name, mode);
        return NULL;
    }

    // file was found
    file_t *f = malloc(sizeof(file_t));
    f->entry = (filestat_t *) fs_dataoffset(fs, 1, offset);

    // is the file exclusively locked?
    for (int i = 0; i < fs->opened_count; ++i) {
        file_t *opened = fs->open_files[i];
        if (opened->entry->blockno != f->entry->blockno) continue;
        // cannot open the file if:
        // - already open in W/A mode
        // - we want to open it in W/A mode
        if (opened->mode != F_READ || mode != F_READ) {
            free(f);
            raise_n(PEINUSE);
        }
    }

    // otherwise, we can continue opening it
    // set up other file struct attrs
    f->offset = mode == F_APPEND ? f->entry->size : 0;
    f->mode = mode;
    f->dirfile_offset = offset;
    // record that we opened the file in fs
    if (fs->opened_count == fs->opened_size) {
        fs->opened_size *= 2;
        fs->open_files = realloc(fs->open_files, sizeof(file_t *) * fs->opened_size);
    }
    fs->open_files[fs->opened_count] = f;
    fs->opened_count++;

    // if the file was found and we are in write mode, truncate it to 0 bytes
    if (mode == F_WRITE) {
        // traverse FAT and set all blocks past first to free
        if (fs->fat[f->entry->blockno] != FAT_EOF) {
            uint16_t blockno = fs->fat[f->entry->blockno];
            fs->fat[f->entry->blockno] = FAT_EOF;
            do {
                uint16_t next_block = fs->fat[blockno];
                fs->fat[blockno] = FAT_FREE;
                blockno = next_block;
            } while (fs->fat[blockno] != FAT_EOF);
        }
        f->entry->size = 0;
    }
    return f;
}

/**
 * Closes the specified file, freeing any associated memory.
 * @param fs the filesystem
 * @param f the file to close
 * @return 0 on a success, -1 on error.
 * @throw PEINVAL the file's entry is invalid
 */
int fs_close(fs_t *fs, file_t *f) {
    unsigned count = 0;
    for (int i = 0; i < fs->opened_count; ++i) {
        // remove it from the fs' list of open files by swapping it with the last open file and decreasing size
        if (fs->open_files[i] == f) {
            // this will segfault if f is not actually open but just don't do that
            file_t *temp = fs->open_files[fs->opened_count - 1];
            fs->open_files[fs->opened_count - 1] = f;
            fs->open_files[i] = temp;
            fs->opened_count--;
        }
        // is this the last instance of this file open?
        if (fs->open_files[i]->entry->blockno == f->entry->blockno) count++;
    }

    // if this was the last open instance of a deleted file (name[0] == 2), delete it now (name[0] = 1, free blocks)
    if (!count && f->entry->name[0] == 2) {
        f->entry->name[0] = 1;
        fs_freeblk(fs, f->entry->blockno);
    }

    // free memory
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
    if (!(f->entry->perm & FAT_READ)) raise(PEFPERM);
    // only read until EOF
    if (f->offset > f->entry->size) return 0;
    if (f->offset + len > f->entry->size)
        len = f->entry->size - f->offset;
    if (!len) return 0;
    ssize_t bytes_read = fs_read_blk(fs, f->entry->blockno, f->offset, len, buf);
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
    if (!(f->entry->perm & FAT_WRITE)) raise(PEFPERM);
    // append mode always seeks to EOF before writing
    if (f->mode == F_APPEND) fs_lseek(fs, f, 0, F_SEEK_END);

    // if the offset is past EOF, fill in hole with null bytes, one block at a time
    while (f->offset > f->entry->size) {
        uint32_t to_write = fs->block_size - (f->entry->size % fs->block_size);
        if (f->entry->size + to_write > f->offset)
            to_write = f->offset - f->entry->size;
        bzero(fs_dataoffset(fs, f->entry->blockno, f->entry->size), to_write);
        f->entry->size += to_write;
    }

    // write to file
    ssize_t bytes_written = fs_write_blk(fs, f->entry->blockno, f->offset, str, len);
    f->offset += bytes_written;
    if (f->offset > f->entry->size) {
        f->entry->size = f->offset;
    }

    // set metadata
    time(&f->entry->mtime);
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
            f->offset = f->entry->size + offset;
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
 * @throw PEHOSTIO failed to i/o with the host entry
 */
int fs_unlink(fs_t *fs, const char *fname) {
    uint32_t offset = fs_find(fs, fname);
    if (offset == -1) return -1;
    // get fileinfo
    filestat_t *f = (filestat_t *) fs_dataoffset(fs, 1, offset);
    // then mark the file as deleted in the root dir
    // is this file still in use?
    // if so, set name[0] to 2; otherwise, set it to 1
    char one = 1;
    for (int i = 0; i < fs->opened_count; ++i) {
        if (fs->open_files[i]->entry->blockno == f->blockno) {
            one = 2;
            break;
        }
    }
    // if we are actually deleting the file now, set the blocks as free
    if (one == 1)
        fs_freeblk(fs, f->blockno);
    f->name[0] = one;
    return 0;
}

/**
 * Gets information for a file. If `fname` is NULL, gets information for all the files.
 * It is the caller's responsibility to free each of the returned structs. Use the convenience function `fs_freels()` to
 * do this quickly.
 * @param fs the filesystem
 * @param fname the name of the file to get the stat of, or NULL to list all files
 * @return a pointer to an array of filestat struct pointers. The array will always be terminated with a NULL pointer.
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
        if (tempf.name[0] == 0 || r == 0) break; // end of directory
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
    } while (true);
    f[count] = NULL;
    return f;
}

/**
 * Rename a file.
 * @param fs the filesystem
 * @param oldname the old name
 * @param newname the new name
 * @return 0 on success, -1 on failure
 * @throw PENOFILE the file does not exist
 * @throw PEHOSTIO failed to perform IO on host drive
 * @throw PEFNAME the new name is invalid
 */
int fs_rename(fs_t *fs, const char *oldname, const char *newname) {
    // does the source file exist?
    uint32_t offset = fs_find(fs, oldname);
    if (offset == -1) return -1;
    // is the new name valid?
    int namelen = strlen(newname); // NOLINT(cppcoreguidelines-narrowing-conversions)
    char letter;
    if (!namelen || namelen > 31) raise(PEFNAME);
    for (int i = 0; i < namelen; ++i) {
        letter = newname[i];
        if (!(isalnum(letter) || letter == '-' || letter == '_' || letter == '.')) raise(PEFNAME);
    }
    // delete a file named *newname* if it exists
    if (fs_unlink(fs, newname) == -1) {
        if (ERRNO != PENOFILE) return -1;
    }
    // ok, write the new name to the file
    filestat_t *f = (filestat_t *) fs_dataoffset(fs, 1, offset);
    strncpy(f->name, newname, FAT_NAME_LEN);
    time(&f->mtime);
    return 0;
}

// ==== low-level helpers ====
/**
 * Creates a new file with the given name in the next open block.
 * @param fs the filesystem
 * @param name the name of the file
 * @param mode the mode to return the newly-created file in
 * @return the new file, NULL on error
 * @throw PEFNAME file name is invalid
 * @throw PEHOSTIO failed to read from/write to host filesystem
 * @throw PETOOFAT the filesystem is full
 */
file_t *fs_makefile(fs_t *fs, const char *name, int mode) {
    // validate filename; must be at least 1 character and alnum/[._-]
    int namelen = strlen(name); // NOLINT(cppcoreguidelines-narrowing-conversions)
    char letter;
    if (!namelen || namelen > 31) raise_n(PEFNAME);
    for (int i = 0; i < namelen; ++i) {
        letter = name[i];
        if (!(isalnum(letter) || letter == '-' || letter == '_' || letter == '.')) raise_n(PEFNAME);
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
    if (r == 0)
        bzero(fs_dataoffset(fs, 1, offset), fs->block_size);

    // find the next free block
    uint16_t block = fs_link_next_free(fs);
    if (!block) raise_n(PETOOFAT);

    // here is where I will make my file
    // create the file object
    file_t *f = malloc(sizeof(file_t));
    filestat_t *entry = fs_dataoffset(fs, 1, offset);
    strncpy(entry->name, name, FAT_NAME_LEN - 1);
    entry->name[FAT_NAME_LEN - 1] = 0;
    entry->blockno = block;
    entry->type = 1;  // regular file
    entry->perm = FAT_READ | FAT_WRITE;  // read/write
    time(&entry->mtime);

    f->entry = entry;
    f->offset = 0;
    f->mode = mode;
    f->dirfile_offset = offset;
    // record that we opened the file in fs
    if (fs->opened_count == fs->opened_size) {
        fs->opened_size *= 2;
        fs->open_files = realloc(fs->open_files, sizeof(file_t *) * fs->opened_size);
    }
    fs->open_files[fs->opened_count] = f;
    fs->opened_count++;
    return f;
}

/**
 * Similar to fs_read, except takes a block number and offset. Traverses the FAT if offset > entry.block_size.
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
 * Similar to fs_read, except takes a block number and offset. Traverses the FAT if offset > entry.block_size.
 * If the write would overflow past the EOF block, allocs a new block.
 * @param fs the filesystem to read from
 * @param blk_base_no the block number the file starts in
 * @param offset where to start reading relative to the base block number
 * @param str a buffer to store the bytes to write
 * @param len the maximum number of bytes to write
 * @return the number of bytes written; -1 on error
 * @throw PEINVAL the provided block number or offset is outside the filesystem
 * @throw PEHOSTIO failed to read from host filesystem
 * @throw PETOOFAT filesystem is full
 */
ssize_t fs_write_blk(fs_t *fs, uint16_t blk_base_no, uint32_t offset, const void *str, uint32_t len) {
    if (blk_base_no >= fs->fat_size / 2 || blk_base_no == 0) raise(PEINVAL);
    // seek to the right spot on host
    if (fs_hostseek(fs, blk_base_no, offset) == -1) return -1;
    offset = offset % fs->block_size;
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
        // write the requested length to the block
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
 * @throw PENOFILE requested file does not exist
 */
uint32_t fs_find(fs_t *fs, const char *fname) {
    // traverse the root directory looking for the file with given name
    uint32_t offset = 0;
    char tempname[FAT_NAME_LEN];
    ssize_t r;
    do {
        r = fs_read_blk(fs, 1, offset, FAT_NAME_LEN, tempname);
        if (r == -1) return -1;
        if (tempname[0] == 0) raise(PENOFILE); // end of directory
        if (strncmp(tempname, fname, FAT_NAME_LEN) == 0) {
            return offset;
        }
        offset += FAT_FILE_SIZE;
    } while (r);
    raise(PENOFILE);
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

/**
 * Seek the fd on the host filesystem to the position specified by the block number + offset. If this operation would
 * go past the EOF of a block, allocates a new block.
 * @param fs the filesystem
 * @param blk_base_no the block number to start from
 * @param offset how far to seek relative to the start of the block number
 * @return the offset on the host filesystem on success, -1 on error
 * @throw PEHOSTIO could not seek on host
 * @throw PETOOFAT the filesystem is full
 */
off_t fs_hostseek(fs_t *fs, uint16_t blk_base_no, uint32_t offset) {
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
    off_t retval = lseek(fs->host_fd, fs->fat_size + fs->block_size * (blk_base_no - 1) + offset, SEEK_SET);
    if (retval == -1) raise(PEHOSTIO);
    return retval;
}

/**
 * Marks all the blocks in the file starting at `blockno` as free in the FAT.
 * @param fs the filesystem
 * @param blockno the block number to start freeing at
 * @return the number of blocks freed
 */
int fs_freeblk(fs_t *fs, uint16_t blockno) {
    int freed = 0;
    do {
        uint16_t next_blockno = fs->fat[blockno];
        fs->fat[blockno] = FAT_FREE;
        freed++;
        blockno = next_blockno;
    } while (blockno != FAT_EOF);
    return freed;
}

/**
 * Return a pointer to the location in the data memory map corresponding to the offset relative to the given block.
 * Traverses the FAT if necessary.
 * @param fs the filesystem
 * @param blk_base_no the block to start at
 * @param offset how for to go
 * @return a pointer to the right place
 * @throw PEINVAL the specified offset is not within the file starting at the given block
 */
void *fs_dataoffset(fs_t *fs, uint16_t blk_base_no, uint32_t offset) {
    // seek to the right offset
    while (offset >= fs->block_size) {
        uint16_t next_block = fs->fat[blk_base_no];
        if (next_block == FAT_EOF) raise_n(PEINVAL);
        offset -= fs->block_size;
        blk_base_no = next_block;
    }
    return fs->data + fs->fat_size + fs->block_size * (blk_base_no - 1) + offset;
}