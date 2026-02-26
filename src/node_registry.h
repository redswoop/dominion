/*
 * node_registry.h — File-based per-node status tracking.
 *
 * Each active node writes a small JSON file to data/nodes/N.json.
 * All operations use FileLock for concurrent safety.
 */

#ifndef NODE_REGISTRY_H
#define NODE_REGISTRY_H

#include <time.h>

#define MAX_NODES 20

struct NodeInfo {
    int node;
    int pid;
    int user_num;
    char user_name[32];
    char status[64];
    time_t since;
};

/* Assign the first available node number (1..max_nodes).
 * Returns -1 if all nodes are busy. */
int node_assign(const char *datadir, int max_nodes);

/* Claim a node: write initial node file with PID. */
void node_claim(const char *datadir, int node_num, int pid);

/* Update node info (after login, menu change, etc.). */
void node_update(const char *datadir, int node_num, int user_num,
                 const char *user_name, const char *status);

/* Release a node: delete its file. */
void node_release(const char *datadir, int node_num);

/* List all active nodes. Returns count, fills info[] up to max_count. */
int node_list(const char *datadir, NodeInfo *info, int max_count);

/* Check if a user number is logged in on any node. */
int node_user_online(const char *datadir, int user_num);

/* Remove entries for dead PIDs (stale cleanup). */
void node_reap_stale(const char *datadir);

#endif /* NODE_REGISTRY_H */
