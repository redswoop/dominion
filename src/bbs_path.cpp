// bbs_path.cpp — Path construction utilities
// Lakos: utility component (static methods only), leaf

#include "bbs_path.h"
#include <sys/stat.h>
#include <cstring>
#include <unistd.h>
#include <strings.h>
#include <filesystem>
#include <fnmatch.h>

std::string BbsPath::join(const char *dir, const char *name) {
    if (!dir) dir = "";
    if (!name) name = "";

    size_t dlen = strlen(dir);
    size_t nlen = strlen(name);

    // Empty dir: return name as-is
    if (dlen == 0) return std::string(name);

    // Empty name: return dir as-is
    if (nlen == 0) return std::string(dir);

    // Strip leading slash from name if dir ends with slash
    bool dir_has_slash = (dir[dlen - 1] == '/');
    bool name_has_slash = (name[0] == '/');

    std::string result;
    result.reserve(dlen + 1 + nlen);
    result.append(dir, dlen);

    if (dir_has_slash && name_has_slash) {
        // Skip the duplicate slash
        result.append(name + 1, nlen - 1);
    } else if (!dir_has_slash && !name_has_slash) {
        // Need to insert a slash
        result += '/';
        result.append(name, nlen);
    } else {
        // Exactly one slash between them
        result.append(name, nlen);
    }

    return result;
}

std::string BbsPath::join(const std::string &dir, const char *name) {
    return join(dir.c_str(), name);
}

std::string BbsPath::join(const char *dir, const std::string &name) {
    return join(dir, name.c_str());
}

std::string BbsPath::join(const std::string &dir, const std::string &name) {
    return join(dir.c_str(), name.c_str());
}

std::string BbsPath::join(const char *dir, const char *mid, const char *name) {
    return join(join(dir, mid), name);
}

bool BbsPath::exists(const std::string &path) {
    return exists(path.c_str());
}

bool BbsPath::exists(const char *path) {
    if (!path) return false;

    /* DOS "nul" device idiom: "path/nul" means "does the directory exist?" */
    const char *base = strrchr(path, '/');
    if (!base) base = strrchr(path, '\\');
    if (base) base++; else base = path;
    if (strcasecmp(base, "nul") == 0) {
        if (base == path) return true;  /* bare "nul" — cwd always exists */
        int len = (int)(base - path - 1);
        if (len <= 0) return true;
        char dir[512];
        memcpy(dir, path, len);
        dir[len] = 0;
        struct stat st;
        return (stat(dir, &st) == 0 && S_ISDIR(st.st_mode));
    }

    return access(path, F_OK) == 0;
}

std::string BbsPath::basename(const std::string &path) {
    if (path.empty()) return "";
    size_t pos = path.rfind('/');
    if (pos == std::string::npos) return path;
    return path.substr(pos + 1);
}

std::string BbsPath::ensure_slash(const std::string &dir) {
    if (dir.empty()) return "/";
    if (dir.back() == '/') return dir;
    return dir + '/';
}

void bbs_filter(char *s, unsigned char c)
{
    int x = 0;
    while (s[x++] != c)
        ;
    s[x-1] = 0;
}

double disk_free_kb(const char *path)
{
    std::error_code ec;
    auto si = std::filesystem::space(path, ec);
    if (ec) return -1.0;
    return (double)si.available / 1024.0;
}

void clear_directory(const char *dir, const char *pattern)
{
    std::error_code ec;
    for (auto &entry : std::filesystem::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file(ec))
            continue;
        if (pattern) {
            auto fname = entry.path().filename().string();
            if (fnmatch(pattern, fname.c_str(), 0) != 0)
                continue;
        }
        std::filesystem::remove(entry.path(), ec);
    }
}
