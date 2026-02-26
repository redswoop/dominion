/*
 * file_lock.cpp — RAII file lock using flock(2) on sidecar .lock files.
 */

#include "file_lock.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>

FileLock::FileLock(const char *path)
{
    char lockpath[512];
    snprintf(lockpath, sizeof(lockpath), "%s.lock", path);

    fd_ = open(lockpath, O_CREAT | O_RDWR, 0644);
    if (fd_ < 0)
        return;

    if (flock(fd_, LOCK_EX) != 0) {
        close(fd_);
        fd_ = -1;
    }
}

FileLock::~FileLock()
{
    if (fd_ >= 0) {
        flock(fd_, LOCK_UN);
        close(fd_);
    }
}
