/*
 * file_lock.h — RAII file lock using flock(2) on sidecar .lock files.
 *
 * Provides exclusive locking for shared data files in the forking
 * multi-user BBS model. Each lock creates/opens a "path.lock" sidecar
 * file and holds LOCK_EX for the lifetime of the FileLock object.
 */

#ifndef FILE_LOCK_H
#define FILE_LOCK_H

class FileLock {
public:
    explicit FileLock(const char *path);
    ~FileLock();

    bool held() const { return fd_ >= 0; }

    FileLock(const FileLock&) = delete;
    FileLock& operator=(const FileLock&) = delete;

private:
    int fd_ = -1;
};

#endif /* FILE_LOCK_H */
