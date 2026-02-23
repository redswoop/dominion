/*
 * menu_json.c — JSON serialization for menu_data_t (mmrec + menurec[]).
 *
 * Converts between in-memory menu structs and human-readable JSON files.
 * The JSON format replaces opaque binary .mnu files with editable text.
 *
 * Uses cJSON library; follows patterns from json_io.c.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "menu_json.h"
#include "json_io.h"   /* read_json_file, write_json_file */


/* ================================================================
 * Helpers
 * ================================================================ */

static int jint(cJSON *obj, const char *key)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsNumber(item))
        return item->valueint;
    return 0;
}

static int jbool(cJSON *obj, const char *key)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsBool(item))
        return cJSON_IsTrue(item);
    return 0;
}

/* Add string to JSON, but use null if empty */
static void add_str_or_null(cJSON *obj, const char *key, const char *val)
{
    if (val && val[0])
        cJSON_AddStringToObject(obj, key, val);
    else
        cJSON_AddNullToObject(obj, key);
}

/* Read string from JSON, treating null as empty */
static void jstr_or_null(cJSON *obj, const char *key, char *dest, int maxlen)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsString(item) && item->valuestring) {
        strncpy(dest, item->valuestring, maxlen - 1);
        dest[maxlen - 1] = '\0';
    } else {
        dest[0] = '\0';
    }
}


/* ================================================================
 * Command attr: bitmask <-> string
 * ================================================================ */

static const char *cmd_attr_to_str(INT16 attr)
{
    if (attr & command_hidden)   return "hidden";
    if (attr & command_unhidden) return "unhidden";
    if (attr & command_title)    return "title";
    if (attr & command_every)    return "every";
    if (attr & command_default)  return "default";
    return NULL;
}

static INT16 str_to_cmd_attr(const char *s)
{
    if (!s) return 0;
    if (strcmp(s, "hidden") == 0)   return command_hidden;
    if (strcmp(s, "unhidden") == 0) return command_unhidden;
    if (strcmp(s, "title") == 0)    return command_title;
    if (strcmp(s, "every") == 0)    return command_every;
    if (strcmp(s, "default") == 0)  return command_default;
    return 0;
}


/* ================================================================
 * Hotkey mode: boarder byte <-> string
 * ================================================================ */

static const char *hotkey_mode_str(char boarder)
{
    if (boarder == 1) return "forced";
    if (boarder == 2) return "off";
    return "normal";
}

static char str_to_hotkey_mode(const char *s)
{
    if (!s) return 0;
    if (strcmp(s, "forced") == 0) return 1;
    if (strcmp(s, "off") == 0)    return 2;
    return 0;
}


/* ================================================================
 * Single command -> JSON
 * ================================================================ */

static cJSON *cmd_to_json(const menurec *cmd)
{
    cJSON *obj = cJSON_CreateObject();
    const char *attr_str;

    cJSON_AddStringToObject(obj, "key", cmd->key);
    cJSON_AddStringToObject(obj, "type", cmd->type);
    add_str_or_null(obj, "param", cmd->ms);
    add_str_or_null(obj, "desc", cmd->desc);
    add_str_or_null(obj, "line", cmd->line);
    add_str_or_null(obj, "access", cmd->sl);

    attr_str = cmd_attr_to_str(cmd->attr);
    if (attr_str)
        cJSON_AddStringToObject(obj, "attr", attr_str);
    else
        cJSON_AddNullToObject(obj, "attr");

    return obj;
}


/* ================================================================
 * JSON -> single command
 * ================================================================ */

static void json_to_cmd(cJSON *obj, menurec *cmd)
{
    cJSON *attr_item;

    memset(cmd, 0, sizeof(menurec));
    jstr_or_null(obj, "key",   cmd->key,  sizeof(cmd->key));
    jstr_or_null(obj, "type",  cmd->type, sizeof(cmd->type));
    jstr_or_null(obj, "param", cmd->ms,   sizeof(cmd->ms));
    jstr_or_null(obj, "desc",  cmd->desc, sizeof(cmd->desc));
    jstr_or_null(obj, "line",  cmd->line, sizeof(cmd->line));
    jstr_or_null(obj, "access",cmd->sl,   sizeof(cmd->sl));

    attr_item = cJSON_GetObjectItemCaseSensitive(obj, "attr");
    if (cJSON_IsString(attr_item) && attr_item->valuestring)
        cmd->attr = str_to_cmd_attr(attr_item->valuestring);
    else
        cmd->attr = 0;
}


/* ================================================================
 * menu_data_t -> JSON
 * ================================================================ */

cJSON *menu_data_to_json(const menu_data_t *data)
{
    cJSON *root, *display, *colors, *flags, *on_enter, *commands;
    const mmrec *h = &data->header;
    int i;
    char id[20];

    root = cJSON_CreateObject();

    /* Derive id from filename (strip .mnu extension) */
    strncpy(id, data->name, sizeof(id) - 1);
    id[sizeof(id) - 1] = '\0';
    {
        char *dot = strrchr(id, '.');
        if (dot) *dot = '\0';
    }
    cJSON_AddStringToObject(root, "id", id);

    /* Header string fields */
    add_str_or_null(root, "title",    h->title1);
    add_str_or_null(root, "subtitle", h->title2);
    cJSON_AddStringToObject(root, "prompt", h->prompt);
    add_str_or_null(root, "ansi_art",  h->altmenu);
    add_str_or_null(root, "access",    h->slneed);
    add_str_or_null(root, "mci_name",  h->pausefile);
    add_str_or_null(root, "help_file", h->helpfile);

    /* Display block */
    display = cJSON_CreateObject();
    cJSON_AddNumberToObject(display, "columns", (int)h->columns);

    colors = cJSON_CreateObject();
    cJSON_AddNumberToObject(colors, "brackets", (int)(unsigned char)h->col[0]);
    cJSON_AddNumberToObject(colors, "keys",     (int)(unsigned char)h->col[1]);
    cJSON_AddNumberToObject(colors, "desc",     (int)(unsigned char)h->col[2]);
    cJSON_AddItemToObject(display, "colors", colors);

    cJSON_AddStringToObject(display, "hotkeys", hotkey_mode_str(h->boarder));
    add_str_or_null(display, "format_file", h->format);
    cJSON_AddItemToObject(root, "display", display);

    /* Flags block */
    flags = cJSON_CreateObject();
    cJSON_AddBoolToObject(flags, "ext_prompt",    (h->attr & menu_extprompt) != 0);
    cJSON_AddBoolToObject(flags, "use_global",    (h->attr & menu_noglobal) == 0);
    cJSON_AddBoolToObject(flags, "hide_global",   (h->attr & menu_hideglobal) != 0);
    cJSON_AddBoolToObject(flags, "prompt_append",  (h->attr & menu_promptappend) != 0);
    if (h->helplevel)
        cJSON_AddNumberToObject(flags, "help_level", (int)h->helplevel);
    else
        cJSON_AddNullToObject(flags, "help_level");
    cJSON_AddItemToObject(root, "flags", flags);

    /* Split commands into on_enter (forced) and regular */
    on_enter = cJSON_CreateArray();
    commands = cJSON_CreateArray();

    for (i = 0; i < data->count; i++) {
        const menurec *cmd = &data->commands[i];
        if (cmd->attr == command_forced) {
            /* on_enter: just type + param + access */
            cJSON *oe = cJSON_CreateObject();
            cJSON_AddStringToObject(oe, "type", cmd->type);
            add_str_or_null(oe, "param", cmd->ms);
            add_str_or_null(oe, "access", cmd->sl);
            cJSON_AddItemToArray(on_enter, oe);
        } else {
            cJSON_AddItemToArray(commands, cmd_to_json(cmd));
        }
    }

    cJSON_AddItemToObject(root, "on_enter", on_enter);
    cJSON_AddItemToObject(root, "commands", commands);

    return root;
}


/* ================================================================
 * JSON -> menu_data_t
 * ================================================================ */

int json_to_menu_data(cJSON *root, menu_data_t *out)
{
    cJSON *display, *colors, *flags, *on_enter, *commands, *item;
    mmrec *h;
    int n;
    char id[20];

    if (!root || !out) return -1;

    memset(out, 0, sizeof(menu_data_t));
    h = &out->header;

    /* id -> name (add .json extension) */
    jstr_or_null(root, "id", id, sizeof(id));
    snprintf(out->name, sizeof(out->name), "%s.json", id);

    /* Header string fields */
    jstr_or_null(root, "title",     h->title1,    sizeof(h->title1));
    jstr_or_null(root, "subtitle",  h->title2,    sizeof(h->title2));
    jstr_or_null(root, "prompt",    h->prompt,    sizeof(h->prompt));
    jstr_or_null(root, "ansi_art",  h->altmenu,   sizeof(h->altmenu));
    jstr_or_null(root, "access",    h->slneed,    sizeof(h->slneed));
    jstr_or_null(root, "mci_name",  h->pausefile,  sizeof(h->pausefile));
    jstr_or_null(root, "help_file", h->helpfile,   sizeof(h->helpfile));

    h->battr = 0;

    /* Display block */
    display = cJSON_GetObjectItemCaseSensitive(root, "display");
    if (display) {
        h->columns = (char)jint(display, "columns");

        colors = cJSON_GetObjectItemCaseSensitive(display, "colors");
        if (colors) {
            h->col[0] = (char)jint(colors, "brackets");
            h->col[1] = (char)jint(colors, "keys");
            h->col[2] = (char)jint(colors, "desc");
        }

        {
            cJSON *hk = cJSON_GetObjectItemCaseSensitive(display, "hotkeys");
            if (cJSON_IsString(hk) && hk->valuestring)
                h->boarder = str_to_hotkey_mode(hk->valuestring);
        }

        jstr_or_null(display, "format_file", h->format, sizeof(h->format));
        if (h->format[0])
            h->attr |= menu_format;
    }

    /* Flags block */
    flags = cJSON_GetObjectItemCaseSensitive(root, "flags");
    if (flags) {
        if (jbool(flags, "ext_prompt"))    h->attr |= menu_extprompt;
        if (!jbool(flags, "use_global"))   h->attr |= menu_noglobal;
        if (jbool(flags, "hide_global"))   h->attr |= menu_hideglobal;
        if (jbool(flags, "prompt_append")) h->attr |= menu_promptappend;

        {
            cJSON *hl = cJSON_GetObjectItemCaseSensitive(flags, "help_level");
            if (cJSON_IsNumber(hl))
                h->helplevel = (char)hl->valueint;
        }
    }

    /* Build command array: on_enter first (forced), then regular */
    n = 0;

    on_enter = cJSON_GetObjectItemCaseSensitive(root, "on_enter");
    if (cJSON_IsArray(on_enter)) {
        cJSON_ArrayForEach(item, on_enter) {
            if (n >= MAX_MENU_COMMANDS) break;
            memset(&out->commands[n], 0, sizeof(menurec));
            jstr_or_null(item, "type",   out->commands[n].type, sizeof(out->commands[n].type));
            jstr_or_null(item, "param",  out->commands[n].ms,   sizeof(out->commands[n].ms));
            jstr_or_null(item, "access", out->commands[n].sl,   sizeof(out->commands[n].sl));
            out->commands[n].attr = command_forced;
            n++;
        }
    }

    commands = cJSON_GetObjectItemCaseSensitive(root, "commands");
    if (cJSON_IsArray(commands)) {
        cJSON_ArrayForEach(item, commands) {
            if (n >= MAX_MENU_COMMANDS) break;
            json_to_cmd(item, &out->commands[n]);
            n++;
        }
    }

    out->count = n;
    return 0;
}


/* ================================================================
 * File-level operations
 * ================================================================ */

int menu_json_load(const char *path, menu_data_t *out)
{
    cJSON *root;
    int rc;

    root = read_json_file(path);
    if (!root)
        return -1;

    rc = json_to_menu_data(root, out);
    cJSON_Delete(root);
    return rc < 0 ? -2 : 0;
}

int menu_json_save(const char *path, const menu_data_t *data)
{
    cJSON *root;
    int rc;

    root = menu_data_to_json(data);
    if (!root)
        return -1;

    rc = write_json_file(path, root);
    cJSON_Delete(root);
    return rc;
}
