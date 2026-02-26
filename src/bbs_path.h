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

#endif
