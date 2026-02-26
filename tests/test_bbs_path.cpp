// test_bbs_path.cpp — Standalone unit tests for BbsPath
// Compile: c++ -std=c++17 -I src -o build/test_bbs_path tests/test_bbs_path.cpp src/bbs_path.cpp
// Run: build/test_bbs_path

#include "bbs_path.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { \
    tests_run++; \
    printf("  %-50s", name); \
} while(0)

#define PASS() do { \
    tests_passed++; \
    printf("OK\n"); \
} while(0)

#define ASSERT_EQ(a, b) do { \
    std::string _a = (a); \
    std::string _b = (b); \
    if (_a != _b) { \
        printf("FAIL\n    expected: \"%s\"\n    got:      \"%s\"\n", _b.c_str(), _a.c_str()); \
        return; \
    } \
} while(0)

#define ASSERT_TRUE(x) do { \
    if (!(x)) { \
        printf("FAIL\n    expected true, got false\n"); \
        return; \
    } \
} while(0)

#define ASSERT_FALSE(x) do { \
    if (x) { \
        printf("FAIL\n    expected false, got true\n"); \
        return; \
    } \
} while(0)

// --- join: basic ---

static void test_join_dir_with_trailing_slash() {
    TEST("join: dir with trailing slash");
    ASSERT_EQ(BbsPath::join("/data/", "user.lst"), "/data/user.lst");
    PASS();
}

static void test_join_dir_without_trailing_slash() {
    TEST("join: dir without trailing slash");
    ASSERT_EQ(BbsPath::join("/data", "user.lst"), "/data/user.lst");
    PASS();
}

// --- join: double-slash prevention ---

static void test_join_double_slash_prevention() {
    TEST("join: double-slash prevention");
    ASSERT_EQ(BbsPath::join("/data/", "/user.lst"), "/data/user.lst");
    PASS();
}

// --- join: empty components ---

static void test_join_empty_dir() {
    TEST("join: empty dir");
    ASSERT_EQ(BbsPath::join("", "user.lst"), "user.lst");
    PASS();
}

static void test_join_empty_name() {
    TEST("join: empty name");
    ASSERT_EQ(BbsPath::join("/data/", ""), "/data/");
    PASS();
}

static void test_join_both_empty() {
    TEST("join: both empty");
    ASSERT_EQ(BbsPath::join("", ""), "");
    PASS();
}

// --- join: no trailing slash on dir ---

static void test_join_gfiles_pattern() {
    TEST("join: gfiles pattern");
    ASSERT_EQ(BbsPath::join("/gfiles", "logon.fmt"), "/gfiles/logon.fmt");
    PASS();
}

// --- join: three-part ---

static void test_join_three_part() {
    TEST("join: three-part basic");
    ASSERT_EQ(BbsPath::join("/data", "dir", "GENERAL.DIR"), "/data/dir/GENERAL.DIR");
    PASS();
}

static void test_join_three_part_slashes() {
    TEST("join: three-part with trailing slashes");
    ASSERT_EQ(BbsPath::join("/data/", "dir/", "GENERAL.DIR"), "/data/dir/GENERAL.DIR");
    PASS();
}

// --- join: realistic BBS patterns ---

static void test_join_bbs_patterns() {
    TEST("join: realistic BBS patterns");
    ASSERT_EQ(BbsPath::join("/bbs/gfiles/", "laston.txt"), "/bbs/gfiles/laston.txt");
    ASSERT_EQ(BbsPath::join("/bbs/data/", "status.json"), "/bbs/data/status.json");
    ASSERT_EQ(BbsPath::join("/bbs/msgs/", "EMAIL"), "/bbs/msgs/EMAIL");
    PASS();
}

// --- join: string overloads ---

static void test_join_string_overloads() {
    TEST("join: string overloads");
    std::string dir = "/data/";
    std::string name = "user.lst";
    ASSERT_EQ(BbsPath::join(dir, "user.lst"), "/data/user.lst");
    ASSERT_EQ(BbsPath::join("/data/", name), "/data/user.lst");
    ASSERT_EQ(BbsPath::join(dir, name), "/data/user.lst");
    PASS();
}

// --- exists ---

static void test_exists_directory() {
    TEST("exists: /tmp directory");
    ASSERT_TRUE(BbsPath::exists("/tmp"));
    PASS();
}

static void test_exists_nonexistent() {
    TEST("exists: nonexistent path");
    ASSERT_FALSE(BbsPath::exists("/nonexistent_path_xyz_12345"));
    PASS();
}

static void test_exists_temp_file() {
    TEST("exists: temp file create/delete");
    char tmp[] = "/tmp/test_bbs_path_XXXXXX";
    int fd = mkstemp(tmp);
    assert(fd >= 0);
    close(fd);
    ASSERT_TRUE(BbsPath::exists(tmp));
    unlink(tmp);
    ASSERT_FALSE(BbsPath::exists(tmp));
    PASS();
}

static void test_exists_nullptr() {
    TEST("exists: nullptr");
    ASSERT_FALSE(BbsPath::exists((const char *)nullptr));
    PASS();
}

static void test_exists_nul_idiom() {
    TEST("exists: DOS nul device idiom");
    // /tmp/nul should return true (directory /tmp exists)
    ASSERT_TRUE(BbsPath::exists("/tmp/nul"));
    // /nonexistent_xyz/nul should return false
    ASSERT_FALSE(BbsPath::exists("/nonexistent_xyz_12345/nul"));
    // bare "nul" means cwd exists → true
    ASSERT_TRUE(BbsPath::exists("nul"));
    PASS();
}

// --- basename ---

static void test_basename_with_path() {
    TEST("basename: /data/user.lst");
    ASSERT_EQ(BbsPath::basename("/data/user.lst"), "user.lst");
    PASS();
}

static void test_basename_no_path() {
    TEST("basename: user.lst (no path)");
    ASSERT_EQ(BbsPath::basename("user.lst"), "user.lst");
    PASS();
}

static void test_basename_trailing_slash() {
    TEST("basename: /data/ (trailing slash)");
    ASSERT_EQ(BbsPath::basename("/data/"), "");
    PASS();
}

static void test_basename_empty() {
    TEST("basename: empty");
    ASSERT_EQ(BbsPath::basename(""), "");
    PASS();
}

// --- ensure_slash ---

static void test_ensure_slash_without() {
    TEST("ensure_slash: /data -> /data/");
    ASSERT_EQ(BbsPath::ensure_slash("/data"), "/data/");
    PASS();
}

static void test_ensure_slash_with() {
    TEST("ensure_slash: /data/ -> /data/");
    ASSERT_EQ(BbsPath::ensure_slash("/data/"), "/data/");
    PASS();
}

static void test_ensure_slash_empty() {
    TEST("ensure_slash: empty -> /");
    ASSERT_EQ(BbsPath::ensure_slash(""), "/");
    PASS();
}

int main() {
    printf("BbsPath unit tests\n");
    printf("==================\n");

    // join
    test_join_dir_with_trailing_slash();
    test_join_dir_without_trailing_slash();
    test_join_double_slash_prevention();
    test_join_empty_dir();
    test_join_empty_name();
    test_join_both_empty();
    test_join_gfiles_pattern();
    test_join_three_part();
    test_join_three_part_slashes();
    test_join_bbs_patterns();
    test_join_string_overloads();

    // exists
    test_exists_directory();
    test_exists_nonexistent();
    test_exists_temp_file();
    test_exists_nullptr();
    test_exists_nul_idiom();

    // basename
    test_basename_with_path();
    test_basename_no_path();
    test_basename_trailing_slash();
    test_basename_empty();

    // ensure_slash
    test_ensure_slash_without();
    test_ensure_slash_with();
    test_ensure_slash_empty();

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
