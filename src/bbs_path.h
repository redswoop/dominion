// bbs_path.h — Path construction utilities
// Lakos: utility component (static methods only), leaf
// Dependencies: <string>, <sys/stat.h>

#ifndef BBS_PATH_H
#define BBS_PATH_H

#include <string>

struct BbsPath {
    // Join directory + filename with exactly one separator.
    //   join("/data/", "user.lst")  -> "/data/user.lst"
    //   join("/data",  "user.lst")  -> "/data/user.lst"
    //   join("/data/", "/user.lst") -> "/data/user.lst"
    //   join("", "user.lst")        -> "user.lst"
    //   join("/data/", "")          -> "/data/"
    static std::string join(const char *dir, const char *name);
    static std::string join(const std::string &dir, const char *name);
    static std::string join(const char *dir, const std::string &name);
    static std::string join(const std::string &dir, const std::string &name);

    // Three-part join: dir + mid + name
    static std::string join(const char *dir, const char *mid, const char *name);

    // File/directory existence check
    static bool exists(const std::string &path);
    static bool exists(const char *path);

    // Extract last path component: "/data/user.lst" -> "user.lst"
    static std::string basename(const std::string &path);

    // Ensure trailing slash: "/data" -> "/data/", "/data/" -> "/data/"
    static std::string ensure_slash(const std::string &dir);
};

// Compat shim — 40+ callsites use exist(), keep it working
inline int exist(const char *s) { return BbsPath::exists(s) ? 1 : 0; }

// String truncation at first occurrence of char c
// Note: platform.h #define filter bbs_filter renames all callsites
void bbs_filter(char *s, unsigned char c);

// Disk free space in KB for filesystem containing path
double disk_free_kb(const char *path);

// Remove files in a directory. If pattern is null, removes all files.
// If pattern is given (e.g. "MYFILE.*"), removes matching files only.
void clear_directory(const char *dir, const char *pattern = nullptr);

#endif
