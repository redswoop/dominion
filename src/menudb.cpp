#include "menudb.h"
#include "menu_json.h"

#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "platform.h"
#include "bbs_path.h"

/* Internal state */
static char db_menudir[MAX_PATH_LEN];


/* Strip extension from key, put bare name into out[20] */
static void bare_key(const char *key, char *out)
{
    const char *dot;
    int len;

    strncpy(out, key, 19);
    out[19] = '\0';
    dot = strrchr(out, '.');
    if (dot) {
        len = dot - out;
        out[len] = '\0';
    }
}

static void make_json_path(const char *bare, char *out)
{
    auto p = BbsPath::join(db_menudir, std::string(bare) + ".json");
    strncpy(out, p.c_str(), MAX_PATH_LEN - 1);
    out[MAX_PATH_LEN - 1] = '\0';
}

static void make_mnu_path(const char *bare, char *out)
{
    auto p = BbsPath::join(db_menudir, std::string(bare) + ".mnu");
    strncpy(out, p.c_str(), MAX_PATH_LEN - 1);
    out[MAX_PATH_LEN - 1] = '\0';
}


void menudb_init(const char *menudir)
{
    strncpy(db_menudir, menudir, MAX_PATH_LEN - 1);
    db_menudir[MAX_PATH_LEN - 1] = '\0';
}


int menudb_exists(const char *key)
{
    char bare[20], path[MAX_PATH_LEN];

    bare_key(key, bare);

    /* Check JSON first, then binary */
    make_json_path(bare, path);
    if (exist(path))
        return 1;

    make_mnu_path(bare, path);
    return exist(path);
}


static int load_binary(const char *path, const char *bare, menu_data_t *out)
{
    int fd, n;

    fd = open(path, O_RDONLY | O_BINARY);
    if (fd < 0)
        return -2;

    memset(out, 0, sizeof(menu_data_t));
    snprintf(out->name, sizeof(out->name), "%s.mnu", bare);

    if (read(fd, &out->header, sizeof(mmrec)) != sizeof(mmrec)) {
        close(fd);
        return -2;
    }

    n = 0;
    while (n < MAX_MENU_COMMANDS &&
           read(fd, &out->commands[n], sizeof(menurec)) == sizeof(menurec)) {
        n++;
    }
    out->count = n;

    close(fd);
    return 0;
}


int menudb_load(const char *key, menu_data_t *out)
{
    char bare[20], path[MAX_PATH_LEN];
    int rc;

    bare_key(key, bare);

    /* Try JSON first */
    make_json_path(bare, path);
    if (exist(path)) {
        rc = menu_json_load(path, out);
        if (rc == 0) {
            /* Normalize name to include .json extension */
            snprintf(out->name, sizeof(out->name), "%s.json", bare);
            return 0;
        }
        /* JSON parse failed — fall through to binary */
    }

    /* Fall back to binary .mnu */
    make_mnu_path(bare, path);
    if (!exist(path))
        return -1;

    return load_binary(path, bare, out);
}


int menudb_create(const char *key, const menu_data_t *data)
{
    char bare[20], path[MAX_PATH_LEN];

    bare_key(key, bare);

    /* Check if either format already exists */
    make_json_path(bare, path);
    if (exist(path))
        return -1;
    make_mnu_path(bare, path);
    if (exist(path))
        return -1;

    /* Create as JSON */
    make_json_path(bare, path);
    return menu_json_save(path, data);
}


int menudb_save(const char *key, const menu_data_t *data)
{
    char bare[20], path[MAX_PATH_LEN];

    bare_key(key, bare);

    /* Always save as JSON */
    make_json_path(bare, path);
    return menu_json_save(path, data);
}


int menudb_delete(const char *key)
{
    char bare[20], path[MAX_PATH_LEN];
    int found = 0;

    bare_key(key, bare);

    /* Delete both formats if they exist */
    make_json_path(bare, path);
    if (exist(path)) {
        unlink(path);
        found = 1;
    }

    make_mnu_path(bare, path);
    if (exist(path)) {
        unlink(path);
        found = 1;
    }

    return found ? 0 : -1;
}


int menudb_list(char names[][20], int max_names)
{
    char pattern[MAX_PATH_LEN];
    struct ffblk f;
    int count = 0;

    /* List .json menus first */
    strcpy(pattern, BbsPath::join(db_menudir, "*.json").c_str());
    if (findfirst(pattern, &f, 0) == 0) {
        do {
            if (count >= max_names) break;
            strncpy(names[count], f.ff_name, 19);
            names[count][19] = '\0';
            count++;
        } while (findnext(&f) != -1);
        if (f._dir) { closedir(f._dir); f._dir = NULL; }
    }

    /* Then .mnu menus (skip if JSON version exists) */
    strcpy(pattern, BbsPath::join(db_menudir, "*.mnu").c_str());
    if (findfirst(pattern, &f, 0) == 0) {
        do {
            char bare[20], json_name[20];
            int already_listed = 0, i;

            if (count >= max_names) break;

            /* Check if JSON version was already listed */
            bare_key(f.ff_name, bare);
            snprintf(json_name, sizeof(json_name), "%s.json", bare);
            for (i = 0; i < count; i++) {
                if (stricmp(names[i], json_name) == 0) {
                    already_listed = 1;
                    break;
                }
            }

            if (!already_listed) {
                strncpy(names[count], f.ff_name, 19);
                names[count][19] = '\0';
                count++;
            }
        } while (findnext(&f) != -1);
        if (f._dir) { closedir(f._dir); f._dir = NULL; }
    }

    return count;
}
