/*
 * node_registry.cpp — File-based per-node status tracking.
 *
 * Each active node has a data/nodes/N.json file containing:
 *   {"node":N, "pid":1234, "user_num":0, "user_name":"", "status":"...", "since":...}
 *
 * All file operations use FileLock for concurrent safety.
 */

#include "node_registry.h"
#include "file_lock.h"
#include "cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>

/* Build path to a node file: datadir/nodes/N.json */
static void node_path(char *buf, int bufsz, const char *datadir, int node_num)
{
    snprintf(buf, bufsz, "%snodes/%d.json", datadir, node_num);
}

/* Write a NodeInfo to its JSON file (caller must hold lock). */
static void write_node_file(const char *path, const NodeInfo *ni)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "node", ni->node);
    cJSON_AddNumberToObject(root, "pid", ni->pid);
    cJSON_AddNumberToObject(root, "user_num", ni->user_num);
    cJSON_AddStringToObject(root, "user_name", ni->user_name);
    cJSON_AddStringToObject(root, "status", ni->status);
    cJSON_AddNumberToObject(root, "since", (double)ni->since);

    char *str = cJSON_Print(root);
    if (str) {
        FILE *f = fopen(path, "w");
        if (f) {
            fputs(str, f);
            fputc('\n', f);
            fclose(f);
        }
        free(str);
    }
    cJSON_Delete(root);
}

/* Read a node file into NodeInfo. Returns 0 on success, -1 on failure. */
static int read_node_file(const char *path, NodeInfo *ni)
{
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (len <= 0) { fclose(f); return -1; }

    char *buf = (char *)malloc(len + 1);
    fread(buf, 1, len, f);
    buf[len] = 0;
    fclose(f);

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) return -1;

    memset(ni, 0, sizeof(*ni));
    cJSON *item;
    item = cJSON_GetObjectItemCaseSensitive(root, "node");
    if (item) ni->node = item->valueint;
    item = cJSON_GetObjectItemCaseSensitive(root, "pid");
    if (item) ni->pid = item->valueint;
    item = cJSON_GetObjectItemCaseSensitive(root, "user_num");
    if (item) ni->user_num = item->valueint;
    item = cJSON_GetObjectItemCaseSensitive(root, "user_name");
    if (item && item->valuestring)
        strncpy(ni->user_name, item->valuestring, sizeof(ni->user_name) - 1);
    item = cJSON_GetObjectItemCaseSensitive(root, "status");
    if (item && item->valuestring)
        strncpy(ni->status, item->valuestring, sizeof(ni->status) - 1);
    item = cJSON_GetObjectItemCaseSensitive(root, "since");
    if (item) ni->since = (time_t)item->valuedouble;

    cJSON_Delete(root);
    return 0;
}


int node_assign(const char *datadir, int max_nodes)
{
    char path[512];
    if (max_nodes > MAX_NODES) max_nodes = MAX_NODES;

    for (int i = 1; i <= max_nodes; i++) {
        node_path(path, sizeof(path), datadir, i);
        FileLock lk(path);
        if (access(path, F_OK) != 0)
            return i;
    }
    return -1;
}

void node_claim(const char *datadir, int node_num, int pid)
{
    char path[512];
    node_path(path, sizeof(path), datadir, node_num);

    FileLock lk(path);
    NodeInfo ni;
    memset(&ni, 0, sizeof(ni));
    ni.node = node_num;
    ni.pid = pid;
    ni.since = time(NULL);
    strncpy(ni.status, "Connecting", sizeof(ni.status) - 1);
    write_node_file(path, &ni);
}

void node_update(const char *datadir, int node_num, int user_num,
                 const char *user_name, const char *status)
{
    char path[512];
    node_path(path, sizeof(path), datadir, node_num);

    FileLock lk(path);
    NodeInfo ni;
    if (read_node_file(path, &ni) != 0) {
        memset(&ni, 0, sizeof(ni));
        ni.node = node_num;
        ni.pid = getpid();
        ni.since = time(NULL);
    }
    ni.user_num = user_num;
    if (user_name) {
        strncpy(ni.user_name, user_name, sizeof(ni.user_name) - 1);
        ni.user_name[sizeof(ni.user_name) - 1] = 0;
    }
    if (status) {
        strncpy(ni.status, status, sizeof(ni.status) - 1);
        ni.status[sizeof(ni.status) - 1] = 0;
    }
    write_node_file(path, &ni);
}

void node_release(const char *datadir, int node_num)
{
    char path[512];
    node_path(path, sizeof(path), datadir, node_num);

    FileLock lk(path);
    unlink(path);
}

int node_list(const char *datadir, NodeInfo *info, int max_count)
{
    char dirpath[512];
    snprintf(dirpath, sizeof(dirpath), "%snodes", datadir);

    DIR *d = opendir(dirpath);
    if (!d) return 0;

    int count = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL && count < max_count) {
        /* Only match N.json files */
        int n = atoi(ent->d_name);
        if (n <= 0) continue;
        char *dot = strrchr(ent->d_name, '.');
        if (!dot || strcmp(dot, ".json") != 0) continue;

        char path[512];
        node_path(path, sizeof(path), datadir, n);
        FileLock lk(path);
        if (read_node_file(path, &info[count]) == 0)
            count++;
    }
    closedir(d);
    return count;
}

int node_user_online(const char *datadir, int user_num)
{
    NodeInfo nodes[MAX_NODES];
    int count = node_list(datadir, nodes, MAX_NODES);
    for (int i = 0; i < count; i++) {
        if (nodes[i].user_num == user_num && nodes[i].pid != getpid())
            return 1;
    }
    return 0;
}

void node_reap_stale(const char *datadir)
{
    NodeInfo nodes[MAX_NODES];
    int count = node_list(datadir, nodes, MAX_NODES);
    for (int i = 0; i < count; i++) {
        if (nodes[i].pid > 0 && kill(nodes[i].pid, 0) != 0) {
            /* Process doesn't exist — stale node file */
            node_release(datadir, nodes[i].node);
        }
    }
}
