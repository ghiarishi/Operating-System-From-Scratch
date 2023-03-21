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
 */
file_t *fs_open(fs_t *fs, const char *name, int mode) {
    // traverse the root directory looking for the file with given name
    uint32_t offset = 0;
    char tempname[FAT_NAME_LEN];
    ssize_t r;
    bool found = false;
    do {
        r = fs_read_blk(fs, 1, offset, FAT_NAME_LEN - 1, tempname);
        tempname[r] = 0;
        if (tempname[0] == 0) break; // end of directory
        if (strcmp(tempname, name) == 0) {
            found = true;
            break;
        };
        offset += FAT_FILE_SIZE;
    } while (r);

    // if the file was not found, create it if in a creation mode
    if (!found) {
        if (mode == F_READ) raise_n(PENOFILE);
        return fs_makefile(fs, name, mode);
    }

    // file was found
    file_t *f = malloc(sizeof(file_t));
    f->offset = 0;
    f->mode = mode;
    r = fs_read_blk(fs, 1, offset, FAT_FILE_SIZE, f);
    if (r == -1) return NULL;
    if (r != FAT_FILE_SIZE) raise_n(PEHOSTIO);
    return f;
}


int fs_close(fs_t *fs, file_t *f);
int fs_read(fs_t *fs, file_t *f, uint32_t len, char *buf);
int fs_write(fs_t *fs, file_t *f, const char *str, uint32_t len);
int fs_lseek(fs_t *fs, file_t *f, int offset, int whence);
int fs_unlink(fs_t *fs, const char *fname);
int fs_ls(fs_t *fs, const char *fname);

// ==== low-level helpers ====
/**
 * Creates a new file with the given name in the next open block.
 * @param fs the filesystem
 * @param name the name of the file
 * @param mode the mode to return the newly-created file in
 * @return the new file, NULL on error
 * @throw PEHOSTIO failed to read from/write to host filesystem
 * @throw PETOOFAT the filesystem is full
 */
file_t *fs_makefile(fs_t *fs, const char *name, int mode) {
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

    // find the next free block
    uint16_t block = 1;
    bool found_free_space = false;
    while (block < fs->fat_size / 2) {
        if (fs->fat[block] == 0) {
            found_free_space = true;
            break;
        }
        block++;
    }
    if (!found_free_space) raise_n(PETOOFAT);

    // here is where I will make my file
    // mark that the block is used
    fs->fat[block] = FAT_EOF;
    // create the file object
    file_t *f = malloc(sizeof(file_t));
    strncpy(f->name, name, 31);
    f->name[31] = 0;
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
    // seek to the right spot
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

// todo
ssize_t fs_write_blk(fs_t *fs, uint16_t blk_base_no, uint32_t offset, void *str, uint32_t len) {
    return 0;
}