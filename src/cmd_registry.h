/*
 * cmd_registry.h — Command type → handler lookup for menu dispatch.
 *
 * Maps 2-char command type codes (e.g. "MR", "FN", "=/") to handler
 * functions.  Replaces the monolithic switch in ex().
 *
 * Handler signature: int handler(char subtype, const char *param)
 *   subtype = type[1]  (the second character of the command type)
 *   param   = the command's parameter string (ms field)
 *   returns 1 = continue menu loop, 0 = navigation happened (reload menu)
 */

#ifndef _CMD_REGISTRY_H_
#define _CMD_REGISTRY_H_

typedef int (*cmd_handler_fn)(char subtype, const char *param);
typedef int (*cmd_direct_fn)(const char *param);

typedef struct {
    char          prefix;       /* type[0]: 'M', 'F', '=', etc. */
    const char   *name;         /* human-readable family name */
    cmd_handler_fn handler;     /* dispatcher for this family */
} cmd_family_t;

typedef struct {
    const char    code[3];      /* full type code: "OI", "OU", etc. */
    const char   *name;
    cmd_direct_fn handler;
} cmd_direct_t;

/* Lookup handler by type[0].  Returns NULL if not found. */
const cmd_family_t *cmd_lookup(char prefix);

/* Execute a command: looks up type[0], calls handler(type[1], param).
 * Returns 1 = continue, 0 = navigation happened, -1 = unknown type. */
int cmd_exec(const char type[3], const char *param);

#endif /* _CMD_REGISTRY_H_ */
