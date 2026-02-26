// bbs_file.h — File operation utilities with diagnostic logging
// Lakos: utility component (static methods only)
// Dependencies: bbs_path.h, <string>, <fcntl.h>

#ifndef BBS_FILE_H
#define BBS_FILE_H

#include <string>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>

struct BbsFile {
    // Open a file. Logs: path, flags, result or error+errno.
    static int open(const std::string &path, int flags);
    static int open(const std::string &path, int flags, int mode);

    // Close a file descriptor.
    static void close(int fd);

    // Read the i-th record of type T from an open fd.
    // Seeks to index * sizeof(T), reads sizeof(T) bytes.
    // Returns true on success.
    template<typename T>
    static bool read_record(int fd, int index, T *out) {
        off_t offset = (off_t)index * (off_t)sizeof(T);
        if (lseek(fd, offset, SEEK_SET) == (off_t)-1) {
            fprintf(stderr, "[FILE] READ_REC fd=%d index=%d %zu bytes seek -> FAILED: %s (errno=%d)\n",
                    fd, index, sizeof(T), strerror(errno), errno);
            return false;
        }
        ssize_t n = ::read(fd, out, sizeof(T));
        if (n != (ssize_t)sizeof(T)) {
            fprintf(stderr, "[FILE] READ_REC fd=%d index=%d %zu bytes -> FAILED: got %zd, %s (errno=%d)\n",
                    fd, index, sizeof(T), n, strerror(errno), errno);
            return false;
        }
        return true;
    }

    // Write the i-th record of type T to an open fd.
    template<typename T>
    static bool write_record(int fd, int index, const T &rec) {
        off_t offset = (off_t)index * (off_t)sizeof(T);
        if (lseek(fd, offset, SEEK_SET) == (off_t)-1) {
            fprintf(stderr, "[FILE] WRITE_REC fd=%d index=%d %zu bytes seek -> FAILED: %s (errno=%d)\n",
                    fd, index, sizeof(T), strerror(errno), errno);
            return false;
        }
        ssize_t n = ::write(fd, &rec, sizeof(T));
        if (n != (ssize_t)sizeof(T)) {
            fprintf(stderr, "[FILE] WRITE_REC fd=%d index=%d %zu bytes -> FAILED: wrote %zd, %s (errno=%d)\n",
                    fd, index, sizeof(T), n, strerror(errno), errno);
            return false;
        }
        return true;
    }

    // Read entire file into a malloc'd buffer. Caller must free().
    // Returns nullptr on error. Sets *len to bytes read.
    static char *read_all(const std::string &path, long *len);

    // Check if a file descriptor is valid (>= 0).
    static bool valid(int fd) { return fd >= 0; }
};

#endif
