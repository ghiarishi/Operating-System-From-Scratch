#include "fat.h"

/**
 * Mount the PennFAT filesystem found at the path.
 * @param path path to the filesystem file on the host os to read
 * @param fs pointer to store the resulting file system struct
 * @return a pointer to the newly mounted filesystem; NULL on failure (sets *ERRNO*)
 * @throw EHOSTFS Could not open the specified file
 * @throw EHOSTIO Could not read or mmap from the host file
 * @throw EBADFS The filesystem file is invalid
 */
fs_t *fs_mount(const char *path) {
    fs_t *fs = malloc(sizeof(fs_t));

    // open host system file
    int fd = open(path, O_RDWR);
    if (fd == -1)
        raise_n(EHOSTFS);
    fs->host_fd = fd;

    // read the first 2 bytes for blockno info and block size
    uint8_t info[2];
    ssize_t bytes_read;
    bytes_read = read(fd, info, 2);
    if (bytes_read == -1)
        raise_n(EHOSTIO);
    else if (bytes_read < 2)
        raise_n(EBADFS);

    // ensure that the filesystem is valid and load in filesystem info
    if (info[0] == 0 || info[0] > 32)
        raise_n(EBADFS);
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
            raise_n(EBADFS);
    }
    fs->fat_size = fs->block_size * info[0];

    // load in the FAT into memory
    if (lseek(fd, 0, SEEK_SET) == -1)
        raise_n(EHOSTIO);
    fs->fat = mmap(NULL, fs->fat_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fs->fat == MAP_FAILED)
        raise_n(EHOSTIO);

    // and we're good
    return fs;
}

/**
 * Unmount a filesystem and free any associated memory.
 * @param fs the fs to free
 * @return 0 normally; -1 on error
 * @throw EHOSTFS could not close the host file for the fs
 */
int fs_unmount(fs_t *fs) {
    // unlink the memory and free the heap-alloced fs
    munmap(fs->fat, fs->fat_size);
    if (close(fs->host_fd) == -1) raise(EHOSTFS);
    free(fs);
    return 0;
}

/**
 * Open a file in the filesystem.
 * @param fs the filesystem
 * @param name 
 * @param mode
 * @return
 */
file_t *fs_open(fs_t *fs, const char *name, int mode){

}


int fs_close(fs_t *fs, file_t *f);
int fs_read(fs_t *fs, file_t *f, uint32_t len, char *buf);
int fs_write(fs_t *fs, file_t *f, const char *str, uint32_t len);
int fs_lseek(fs_t *fs, file_t *f, int offset, int whence);
int fs_unlink(fs_t *fs, const char *fname);
int fs_ls(fs_t *fs, const char *fname);