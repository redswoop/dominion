#include "menudb.h"

#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "platform.h"

/* exist() is in disk.c, declared in fcns_user.h */
int exist(char *s);

/* Internal state */
static char db_menudir[MAX_PATH_LEN];


static void normalize_key(const char *key, char *out)
{
    strncpy(out, key, 19);
    out[19] = '\0';
    if (!strchr(out, '.'))
        strcat(out, ".mnu");
}


static void make_path(const char *normalized, char *out)
{
    sprintf(out, "%s%s", db_menudir, normalized);
}


void menudb_init(const char *menudir)
{
    strncpy(db_menudir, menudir, MAX_PATH_LEN - 1);
    db_menudir[MAX_PATH_LEN - 1] = '\0';
}


int menudb_exists(const char *key)
{
    char fn[20], path[MAX_PATH_LEN];

    normalize_key(key, fn);
    make_path(fn, path);
    return exist(path);
}


int menudb_load(const char *key, menu_data_t *out)
{
    char fn[20], path[MAX_PATH_LEN];
    int fd, n;

    normalize_key(key, fn);
    make_path(fn, path);
    if (!exist(path))
        return -1;

    fd = open(path, O_RDONLY | O_BINARY);
    if (fd < 0)
        return -2;

    memset(out, 0, sizeof(menu_data_t));
    strncpy(out->name, fn, sizeof(out->name) - 1);

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


int menudb_create(const char *key, const menu_data_t *data)
{
    char fn[20], path[MAX_PATH_LEN];
    int fd;

    normalize_key(key, fn);
    make_path(fn, path);
    if (exist(path))
        return -1;

    fd = open(path, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
    if (fd < 0)
        return -1;

    write(fd, &data->header, sizeof(mmrec));
    write(fd, data->commands, sizeof(menurec) * data->count);
    close(fd);
    return 0;
}


int menudb_save(const char *key, const menu_data_t *data)
{
    char fn[20], path[MAX_PATH_LEN];
    int fd;

    normalize_key(key, fn);
    make_path(fn, path);

    fd = open(path, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
    if (fd < 0)
        return -1;

    write(fd, &data->header, sizeof(mmrec));
    write(fd, data->commands, sizeof(menurec) * data->count);
    close(fd);
    return 0;
}


int menudb_delete(const char *key)
{
    char fn[20], path[MAX_PATH_LEN];

    normalize_key(key, fn);
    make_path(fn, path);
    if (!exist(path))
        return -1;

    unlink(path);
    return 0;
}


int menudb_list(char names[][20], int max_names)
{
    char pattern[MAX_PATH_LEN];
    struct ffblk f;
    int count;

    sprintf(pattern, "%s*.mnu", db_menudir);
    if (findfirst(pattern, &f, 0) != 0)
        return 0;

    count = 0;
    do {
        if (count >= max_names)
            break;
        strncpy(names[count], f.ff_name, 19);
        names[count][19] = '\0';
        count++;
    } while (findnext(&f) != -1);

    return count;
}
