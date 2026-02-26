// bbs_file.cpp — File operation utilities with diagnostic logging
// Lakos: utility component (static methods only)

#include "bbs_file.h"
#include <cstdlib>
#include <sys/stat.h>

// Describe open flags for logging
static const char *describe_flags(int flags) {
    int access = flags & O_ACCMODE;
    if (access == O_RDONLY) return "O_RDONLY";
    if (access == O_WRONLY) return "O_WRONLY";
    if (access == O_RDWR)  return "O_RDWR";
    return "O_???";
}

int BbsFile::open(const std::string &path, int flags) {
    int fd = ::open(path.c_str(), flags);
    if (fd < 0) {
        fprintf(stderr, "[FILE] OPEN %s flags=%s -> FAILED: %s (errno=%d)\n",
                path.c_str(), describe_flags(flags), strerror(errno), errno);
    }
    return fd;
}

int BbsFile::open(const std::string &path, int flags, int mode) {
    int fd = ::open(path.c_str(), flags, mode);
    if (fd < 0) {
        fprintf(stderr, "[FILE] OPEN %s flags=%s mode=0%o -> FAILED: %s (errno=%d)\n",
                path.c_str(), describe_flags(flags), mode, strerror(errno), errno);
    }
    return fd;
}

void BbsFile::close(int fd) {
    if (fd >= 0) {
        ::close(fd);
    }
}

char *BbsFile::read_all(const std::string &path, long *len) {
    if (len) *len = 0;

    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "[FILE] READ_ALL %s -> FAILED: %s (errno=%d)\n",
                path.c_str(), strerror(errno), errno);
        return nullptr;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        fprintf(stderr, "[FILE] READ_ALL %s -> FAILED fstat: %s (errno=%d)\n",
                path.c_str(), strerror(errno), errno);
        ::close(fd);
        return nullptr;
    }

    long size = (long)st.st_size;
    char *buf = (char *)malloc(size + 1);
    if (!buf) {
        fprintf(stderr, "[FILE] READ_ALL %s -> FAILED malloc(%ld)\n",
                path.c_str(), size + 1);
        ::close(fd);
        return nullptr;
    }

    ssize_t n = ::read(fd, buf, size);
    ::close(fd);

    if (n != size) {
        fprintf(stderr, "[FILE] READ_ALL %s -> FAILED: read %zd of %ld bytes\n",
                path.c_str(), n, size);
        free(buf);
        return nullptr;
    }

    buf[size] = '\0';
    if (len) *len = size;
    return buf;
}
