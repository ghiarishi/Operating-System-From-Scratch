#include "errno.h"

int ERRNO = 0;


/**
 * Print the friendly error description to stderr.
 * @param prefix a prefix to put in front of the description
 */
void p_perror(const char *prefix) {
    char *msg;
    switch (ERRNO) {
        // general
        case PENOMEM:
            msg = "out of memory";
            break;
        case PEINVAL:
            msg = "invalid argument";
            break;
            // filesystem
        case PEHOSTFS:
            msg = "could not open/close file in host filesystem";
            break;
        case PEHOSTIO:
            msg = "could not perform I/O in host filesystem";
            break;
        case PEBADFS:
            msg = "invalid PennFAT file, or was otherwise unable to mount";
            break;
        case PENOFILE:
            msg = "specified file does not exist";
            break;
        case PEINUSE:
            msg = "the specified file is in use in another context and an exclusive operation was called";
            break;
        case PETOOFAT:
            msg = "the filesystem is full";
            break;
        case PEFMODE:
            msg = "attempted operation on a file in the wrong mode";
            break;
        case PEFPERM:
            msg = "attempted operation on a file without read/write permissions";
            break;
        case PEFNAME:
            msg = "invalid filename";
            break;
        case PETOOMANYF:
            msg = "too many open files";
            break;
        case PESTDIO:
            msg = "cannot read from stdout or write to stdin";
            break;
            // default
        default:
            msg = "unknown error";
    }
    fprintf(stderr, "%s: %s\n", prefix, msg);
}