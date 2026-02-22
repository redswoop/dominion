/*
 * json_io.c — JSON serialization layer for BBS data files.
 *
 * Converts between in-memory C structs (userrec, configrec, niftyrec,
 * statusrec) and human-readable JSON files on disk. The structs themselves
 * are unchanged — only the I/O format changes from raw binary to JSON.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "json_io.h"

/* ================================================================
 * Helpers
 * ================================================================ */

static long file_size(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0)
        return -1;
    return (long)st.st_size;
}

cJSON *read_json_file(const char *path)
{
    FILE *f;
    long len;
    char *buf;
    cJSON *root;

    f = fopen(path, "r");
    if (!f)
        return NULL;

    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (len <= 0) {
        fclose(f);
        return NULL;
    }

    buf = (char *)malloc(len + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);

    root = cJSON_Parse(buf);
    free(buf);
    return root;
}

int write_json_file(const char *path, cJSON *root)
{
    FILE *f;
    char *str;

    str = cJSON_Print(root);
    if (!str)
        return -1;

    f = fopen(path, "w");
    if (!f) {
        free(str);
        return -1;
    }

    fputs(str, f);
    fputc('\n', f);
    fclose(f);
    free(str);
    return 0;
}

/* Get a string from JSON, copy into fixed-size buffer with null termination */
static void jstr(cJSON *obj, const char *key, char *dest, int maxlen)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsString(item) && item->valuestring) {
        strncpy(dest, item->valuestring, maxlen - 1);
        dest[maxlen - 1] = '\0';
    }
}

/* Get an integer from JSON */
static int jint(cJSON *obj, const char *key)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsNumber(item))
        return item->valueint;
    return 0;
}

/* Get an unsigned long from JSON (cJSON stores doubles internally) */
static unsigned long julong(cJSON *obj, const char *key)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsNumber(item))
        return (unsigned long)item->valuedouble;
    return 0;
}

/* Get a float from JSON */
static float jfloat(cJSON *obj, const char *key)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsNumber(item))
        return (float)item->valuedouble;
    return 0.0f;
}

/* Get a single char from JSON (stored as string) */
static char jchar(cJSON *obj, const char *key)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsString(item) && item->valuestring && item->valuestring[0])
        return item->valuestring[0];
    if (cJSON_IsNumber(item))
        return (char)item->valueint;
    return 0;
}

/* Add a number array to a JSON object */
static void add_uchar_array(cJSON *obj, const char *key,
                            unsigned char *arr, int count)
{
    int i;
    cJSON *a = cJSON_CreateArray();
    for (i = 0; i < count; i++)
        cJSON_AddItemToArray(a, cJSON_CreateNumber(arr[i]));
    cJSON_AddItemToObject(obj, key, a);
}

/* Read a number array from JSON into unsigned char buffer */
static void get_uchar_array(cJSON *obj, const char *key,
                            unsigned char *arr, int count)
{
    cJSON *a = cJSON_GetObjectItemCaseSensitive(obj, key);
    cJSON *item;
    int i = 0;
    if (!cJSON_IsArray(a))
        return;
    cJSON_ArrayForEach(item, a) {
        if (i >= count)
            break;
        if (cJSON_IsNumber(item))
            arr[i] = (unsigned char)item->valueint;
        i++;
    }
}

static void add_ushort_array(cJSON *obj, const char *key,
                             unsigned short *arr, int count)
{
    int i;
    cJSON *a = cJSON_CreateArray();
    for (i = 0; i < count; i++)
        cJSON_AddItemToArray(a, cJSON_CreateNumber(arr[i]));
    cJSON_AddItemToObject(obj, key, a);
}

static void get_ushort_array(cJSON *obj, const char *key,
                             unsigned short *arr, int count)
{
    cJSON *a = cJSON_GetObjectItemCaseSensitive(obj, key);
    cJSON *item;
    int i = 0;
    if (!cJSON_IsArray(a))
        return;
    cJSON_ArrayForEach(item, a) {
        if (i >= count)
            break;
        if (cJSON_IsNumber(item))
            arr[i] = (unsigned short)item->valueint;
        i++;
    }
}

static void add_long_array(cJSON *obj, const char *key,
                           long *arr, int count)
{
    int i;
    cJSON *a = cJSON_CreateArray();
    for (i = 0; i < count; i++)
        cJSON_AddItemToArray(a, cJSON_CreateNumber((double)arr[i]));
    cJSON_AddItemToObject(obj, key, a);
}

static void get_long_array(cJSON *obj, const char *key,
                           long *arr, int count)
{
    cJSON *a = cJSON_GetObjectItemCaseSensitive(obj, key);
    cJSON *item;
    int i = 0;
    if (!cJSON_IsArray(a))
        return;
    cJSON_ArrayForEach(item, a) {
        if (i >= count)
            break;
        if (cJSON_IsNumber(item))
            arr[i] = (long)item->valuedouble;
        i++;
    }
}

static void add_int_array(cJSON *obj, const char *key,
                          int *arr, int count)
{
    int i;
    cJSON *a = cJSON_CreateArray();
    for (i = 0; i < count; i++)
        cJSON_AddItemToArray(a, cJSON_CreateNumber(arr[i]));
    cJSON_AddItemToObject(obj, key, a);
}

static void get_int_array(cJSON *obj, const char *key,
                          int *arr, int count)
{
    cJSON *a = cJSON_GetObjectItemCaseSensitive(obj, key);
    cJSON *item;
    int i = 0;
    if (!cJSON_IsArray(a))
        return;
    cJSON_ArrayForEach(item, a) {
        if (i >= count)
            break;
        if (cJSON_IsNumber(item))
            arr[i] = item->valueint;
        i++;
    }
}

static void add_float_array(cJSON *obj, const char *key,
                            float *arr, int count)
{
    int i;
    cJSON *a = cJSON_CreateArray();
    for (i = 0; i < count; i++)
        cJSON_AddItemToArray(a, cJSON_CreateNumber((double)arr[i]));
    cJSON_AddItemToObject(obj, key, a);
}

static void get_float_array(cJSON *obj, const char *key,
                            float *arr, int count)
{
    cJSON *a = cJSON_GetObjectItemCaseSensitive(obj, key);
    cJSON *item;
    int i = 0;
    if (!cJSON_IsArray(a))
        return;
    cJSON_ArrayForEach(item, a) {
        if (i >= count)
            break;
        if (cJSON_IsNumber(item))
            arr[i] = (float)item->valuedouble;
        i++;
    }
}

/* Add a char array (byte values, not a string) to JSON */
static void add_char_byte_array(cJSON *obj, const char *key,
                                char *arr, int count)
{
    int i;
    cJSON *a = cJSON_CreateArray();
    for (i = 0; i < count; i++)
        cJSON_AddItemToArray(a, cJSON_CreateNumber((int)(unsigned char)arr[i]));
    cJSON_AddItemToObject(obj, key, a);
}

static void get_char_byte_array(cJSON *obj, const char *key,
                                char *arr, int count)
{
    cJSON *a = cJSON_GetObjectItemCaseSensitive(obj, key);
    cJSON *item;
    int i = 0;
    if (!cJSON_IsArray(a))
        return;
    cJSON_ArrayForEach(item, a) {
        if (i >= count)
            break;
        if (cJSON_IsNumber(item))
            arr[i] = (char)item->valueint;
        i++;
    }
}

/* ================================================================
 * Bitmask flag helpers — for human-readable _flags arrays
 * ================================================================ */

typedef struct {
    unsigned long bit;
    const char *name;
} flagdef;

static void add_flags(cJSON *obj, const char *key,
                      unsigned long val, flagdef *defs, int ndefs)
{
    int i;
    cJSON *a = cJSON_CreateArray();
    for (i = 0; i < ndefs; i++) {
        if (val & defs[i].bit)
            cJSON_AddItemToArray(a, cJSON_CreateString(defs[i].name));
    }
    cJSON_AddItemToObject(obj, key, a);
}

/* sysstatus flags */
static flagdef sysstatus_flags[] = {
    { 0x0001, "ansi" },
    { 0x0002, "color" },
    { 0x0004, "fullline" },
    { 0x0008, "pause_on_page" },
    { 0x0010, "rip" },
    { 0x0020, "smw" },
    { 0x0040, "full_screen" },
    { 0x0080, "nscan_file_system" },
    { 0x0100, "regular" },
    { 0x0200, "clr_scrn" },
    { 0x0400, "avatar" }
};
#define N_SYSSTATUS_FLAGS 11

/* restrict flags */
static flagdef restrict_flags[] = {
    { 0x0001, "logon" },
    { 0x0002, "chat" },
    { 0x0004, "validate" },
    { 0x0008, "automessage" },
    { 0x0010, "anony" },
    { 0x0020, "post" },
    { 0x0040, "email" },
    { 0x0080, "vote" },
    { 0x0100, "auto_msg_delete" },
    { 0x0200, "net" },
    { 0x0400, "upload" },
    { 0x0800, "rumours" },
    { 0x1000, "timebank" },
    { 0x2000, "bbslist" },
    { 0x4000, "userlist" }
};
#define N_RESTRICT_FLAGS 15

/* inact flags */
static flagdef inact_flags[] = {
    { 0x01, "deleted" },
    { 0x02, "inactive" },
    { 0x04, "lockedout" }
};
#define N_INACT_FLAGS 3

/* exempt flags */
static flagdef exempt_flags[] = {
    { 0x01, "ratio" },
    { 0x02, "time" },
    { 0x04, "userlist" },
    { 0x08, "post" }
};
#define N_EXEMPT_FLAGS 4

/* sysconfig flags */
static flagdef sysconfig_flags[] = {
    { 0x0001, "no_local" },
    { 0x0002, "no_beep" },
    { 0x0004, "high_speed" },
    { 0x0008, "off_hook" },
    { 0x0010, "two_color" },
    { 0x0020, "flow_control" },
    { 0x0040, "printer" },
    { 0x0080, "list" },
    { 0x0100, "no_xfer" },
    { 0x0200, "2_way" },
    { 0x0400, "no_alias" },
    { 0x0800, "all_sysop" },
    { 0x1000, "shrink_term" },
    { 0x2000, "free_phone" },
    { 0x4000, "log_dl" }
};
#define N_SYSCONFIG_FLAGS 15

/* nifstatus flags */
static flagdef nifstatus_flags[] = {
    { 0x0001, "fpts" },
    { 0x0002, "nuv" },
    { 0x0004, "chattype" },
    { 0x0008, "comment" },
    { 0x0010, "mciok" },
    { 0x0020, "autochat" },
    { 0x0040, "ratio" },
    { 0x0080, "forcevote" },
    { 0x0100, "pcr" },
    { 0x0200, "mpl" },
    { 0x0400, "autocredit" },
    { 0x0800, "automsg" },
    { 0x1000, "lastfew" },
    { 0x2000, "yourinfo" }
};
#define N_NIFSTATUS_FLAGS 14

/* AR/DAR flags (A-P) */
static const char *ar_letters = "ABCDEFGHIJKLMNOP";

static void add_ar_flags(cJSON *obj, const char *key, unsigned short val)
{
    int i;
    cJSON *a = cJSON_CreateArray();
    for (i = 0; i < 16; i++) {
        if (val & (1 << i)) {
            char s[2];
            s[0] = ar_letters[i];
            s[1] = '\0';
            cJSON_AddItemToArray(a, cJSON_CreateString(s));
        }
    }
    cJSON_AddItemToObject(obj, key, a);
}

/* ================================================================
 * userrec <-> JSON
 * ================================================================ */

cJSON *userrec_to_json(userrec *u)
{
    cJSON *root = cJSON_CreateObject();
    int i;

    /* String fields */
    cJSON_AddStringToObject(root, "name", u->name);
    cJSON_AddStringToObject(root, "realname", u->realname);
    cJSON_AddStringToObject(root, "callsign", u->callsign);
    cJSON_AddStringToObject(root, "phone", u->phone);
    cJSON_AddStringToObject(root, "dphone", u->dphone);
    cJSON_AddStringToObject(root, "pw", u->pw);
    cJSON_AddStringToObject(root, "laston", u->laston);
    cJSON_AddStringToObject(root, "firston", u->firston);
    cJSON_AddStringToObject(root, "note", u->note);
    cJSON_AddStringToObject(root, "comment", u->comment);
    cJSON_AddStringToObject(root, "street", u->street);
    cJSON_AddStringToObject(root, "city", u->city);
    {
        char sex_str[2];
        sex_str[0] = u->sex;
        sex_str[1] = '\0';
        cJSON_AddStringToObject(root, "sex", sex_str);
    }

    /* Macros — 4 string fields */
    {
        cJSON *macros = cJSON_CreateArray();
        for (i = 0; i < 4; i++)
            cJSON_AddItemToArray(macros, cJSON_CreateString(u->macros[i]));
        cJSON_AddItemToObject(root, "macros", macros);
    }

    /* Unsigned char scalars */
    cJSON_AddNumberToObject(root, "age", u->age);
    cJSON_AddNumberToObject(root, "inact", u->inact);
    add_flags(root, "inact_flags", u->inact, inact_flags, N_INACT_FLAGS);
    cJSON_AddNumberToObject(root, "comp_type", u->comp_type);
    cJSON_AddNumberToObject(root, "defprot", u->defprot);
    cJSON_AddNumberToObject(root, "defed", u->defed);
    cJSON_AddNumberToObject(root, "flisttype", u->flisttype);
    cJSON_AddNumberToObject(root, "mlisttype", u->mlisttype);
    cJSON_AddNumberToObject(root, "helplevel", u->helplevel);
    cJSON_AddNumberToObject(root, "lastsub", u->lastsub);
    cJSON_AddNumberToObject(root, "lastdir", u->lastdir);
    cJSON_AddNumberToObject(root, "lastconf", u->lastconf);
    cJSON_AddNumberToObject(root, "screenchars", u->screenchars);
    cJSON_AddNumberToObject(root, "screenlines", u->screenlines);
    cJSON_AddNumberToObject(root, "sl", u->sl);
    cJSON_AddNumberToObject(root, "dsl", u->dsl);
    cJSON_AddNumberToObject(root, "exempt", u->exempt);
    add_flags(root, "exempt_flags", u->exempt, exempt_flags, N_EXEMPT_FLAGS);
    add_uchar_array(root, "colors", u->colors, 20);
    add_uchar_array(root, "votes", u->votes, 20);
    cJSON_AddNumberToObject(root, "illegal", u->illegal);
    cJSON_AddNumberToObject(root, "waiting", u->waiting);
    cJSON_AddNumberToObject(root, "subop", u->subop);
    cJSON_AddNumberToObject(root, "ontoday", u->ontoday);

    /* Unsigned short scalars */
    cJSON_AddNumberToObject(root, "forwardusr", u->forwardusr);
    cJSON_AddNumberToObject(root, "msgpost", u->msgpost);
    cJSON_AddNumberToObject(root, "emailsent", u->emailsent);
    cJSON_AddNumberToObject(root, "feedbacksent", u->feedbacksent);
    cJSON_AddNumberToObject(root, "posttoday", u->posttoday);
    cJSON_AddNumberToObject(root, "etoday", u->etoday);
    cJSON_AddNumberToObject(root, "ar", u->ar);
    add_ar_flags(root, "ar_flags", u->ar);
    cJSON_AddNumberToObject(root, "dar", u->dar);
    add_ar_flags(root, "dar_flags", u->dar);
    cJSON_AddNumberToObject(root, "restrict", u->restrict);
    add_flags(root, "restrict_flags", u->restrict, restrict_flags, N_RESTRICT_FLAGS);
    cJSON_AddNumberToObject(root, "month", u->month);
    cJSON_AddNumberToObject(root, "day", u->day);
    cJSON_AddNumberToObject(root, "year", u->year);

    /* int */
    cJSON_AddNumberToObject(root, "fpts", u->fpts);

    /* More unsigned shorts */
    cJSON_AddNumberToObject(root, "uploaded", u->uploaded);
    cJSON_AddNumberToObject(root, "downloaded", u->downloaded);
    cJSON_AddNumberToObject(root, "logons", u->logons);
    cJSON_AddNumberToObject(root, "fsenttoday1", u->fsenttoday1);
    cJSON_AddNumberToObject(root, "emailnet", u->emailnet);
    cJSON_AddNumberToObject(root, "postnet", u->postnet);

    /* Unsigned longs */
    cJSON_AddNumberToObject(root, "msgread", (double)u->msgread);
    cJSON_AddNumberToObject(root, "uk", (double)u->uk);
    cJSON_AddNumberToObject(root, "dk", (double)u->dk);
    cJSON_AddNumberToObject(root, "daten", (double)u->daten);
    cJSON_AddNumberToObject(root, "sysstatus", (double)u->sysstatus);
    add_flags(root, "sysstatus_flags", u->sysstatus, sysstatus_flags, N_SYSSTATUS_FLAGS);
    cJSON_AddNumberToObject(root, "lastrate", (double)u->lastrate);
    cJSON_AddNumberToObject(root, "nuv", (double)u->nuv);
    cJSON_AddNumberToObject(root, "timebank", (double)u->timebank);

    /* Floats */
    cJSON_AddNumberToObject(root, "timeontoday", (double)u->timeontoday);
    cJSON_AddNumberToObject(root, "extratime", (double)u->extratime);
    cJSON_AddNumberToObject(root, "timeon", (double)u->timeon);
    cJSON_AddNumberToObject(root, "pcr", (double)u->pcr);
    cJSON_AddNumberToObject(root, "ratio", (double)u->ratio);
    cJSON_AddNumberToObject(root, "pos_account", (double)u->pos_account);
    cJSON_AddNumberToObject(root, "neg_account", (double)u->neg_account);

    /* Reserved arrays — only include if non-zero */
    {
        int has_res = 0;
        cJSON *reserved;
        for (i = 0; i < 29; i++)
            if (u->res[i] || u->resl[i] || u->resi[i] ||
                u->resf[i] != 0.0f)
                has_res = 1;
        if (has_res) {
            reserved = cJSON_CreateObject();
            add_char_byte_array(reserved, "res", u->res, 29);
            add_long_array(reserved, "resl", u->resl, 29);
            add_int_array(reserved, "resi", u->resi, 29);
            add_float_array(reserved, "resf", u->resf, 29);
            cJSON_AddItemToObject(root, "_reserved", reserved);
        }
    }

    /* qscn and nscn — message scan pointers */
    add_long_array(root, "qscn", u->qscn, 200);
    add_long_array(root, "nscn", u->nscn, 200);

    return root;
}

void json_to_userrec(cJSON *j, userrec *u)
{
    cJSON *item, *reserved;
    int i;

    memset(u, 0, sizeof(userrec));

    /* Strings */
    jstr(j, "name", u->name, sizeof(u->name));
    jstr(j, "realname", u->realname, sizeof(u->realname));
    jstr(j, "callsign", u->callsign, sizeof(u->callsign));
    jstr(j, "phone", u->phone, sizeof(u->phone));
    jstr(j, "dphone", u->dphone, sizeof(u->dphone));
    jstr(j, "pw", u->pw, sizeof(u->pw));
    jstr(j, "laston", u->laston, sizeof(u->laston));
    jstr(j, "firston", u->firston, sizeof(u->firston));
    jstr(j, "note", u->note, sizeof(u->note));
    jstr(j, "comment", u->comment, sizeof(u->comment));
    jstr(j, "street", u->street, sizeof(u->street));
    jstr(j, "city", u->city, sizeof(u->city));
    u->sex = jchar(j, "sex");

    /* Macros */
    item = cJSON_GetObjectItemCaseSensitive(j, "macros");
    if (cJSON_IsArray(item)) {
        cJSON *m;
        i = 0;
        cJSON_ArrayForEach(m, item) {
            if (i >= 4) break;
            if (cJSON_IsString(m) && m->valuestring) {
                strncpy(u->macros[i], m->valuestring, MAX_PATH_LEN - 1);
                u->macros[i][MAX_PATH_LEN - 1] = '\0';
            }
            i++;
        }
    }

    /* Unsigned char scalars */
    u->age = (unsigned char)jint(j, "age");
    u->inact = (unsigned char)jint(j, "inact");
    u->comp_type = (unsigned char)jint(j, "comp_type");
    u->defprot = (unsigned char)jint(j, "defprot");
    u->defed = (unsigned char)jint(j, "defed");
    u->flisttype = (unsigned char)jint(j, "flisttype");
    u->mlisttype = (unsigned char)jint(j, "mlisttype");
    u->helplevel = (unsigned char)jint(j, "helplevel");
    u->lastsub = (unsigned char)jint(j, "lastsub");
    u->lastdir = (unsigned char)jint(j, "lastdir");
    u->lastconf = (unsigned char)jint(j, "lastconf");
    u->screenchars = (unsigned char)jint(j, "screenchars");
    u->screenlines = (unsigned char)jint(j, "screenlines");
    u->sl = (unsigned char)jint(j, "sl");
    u->dsl = (unsigned char)jint(j, "dsl");
    u->exempt = (unsigned char)jint(j, "exempt");
    get_uchar_array(j, "colors", u->colors, 20);
    get_uchar_array(j, "votes", u->votes, 20);
    u->illegal = (unsigned char)jint(j, "illegal");
    u->waiting = (unsigned char)jint(j, "waiting");
    u->subop = (unsigned char)jint(j, "subop");
    u->ontoday = (unsigned char)jint(j, "ontoday");

    /* Unsigned short scalars */
    u->forwardusr = (unsigned short)jint(j, "forwardusr");
    u->msgpost = (unsigned short)jint(j, "msgpost");
    u->emailsent = (unsigned short)jint(j, "emailsent");
    u->feedbacksent = (unsigned short)jint(j, "feedbacksent");
    u->posttoday = (unsigned short)jint(j, "posttoday");
    u->etoday = (unsigned short)jint(j, "etoday");
    u->ar = (unsigned short)jint(j, "ar");
    u->dar = (unsigned short)jint(j, "dar");
    u->restrict = (unsigned short)jint(j, "restrict");
    u->month = (unsigned short)jint(j, "month");
    u->day = (unsigned short)jint(j, "day");
    u->year = (unsigned short)jint(j, "year");

    /* int */
    u->fpts = jint(j, "fpts");

    /* More unsigned shorts */
    u->uploaded = (unsigned short)jint(j, "uploaded");
    u->downloaded = (unsigned short)jint(j, "downloaded");
    u->logons = (unsigned short)jint(j, "logons");
    u->fsenttoday1 = (unsigned short)jint(j, "fsenttoday1");
    u->emailnet = (unsigned short)jint(j, "emailnet");
    u->postnet = (unsigned short)jint(j, "postnet");

    /* Unsigned longs */
    u->msgread = julong(j, "msgread");
    u->uk = julong(j, "uk");
    u->dk = julong(j, "dk");
    u->daten = julong(j, "daten");
    u->sysstatus = julong(j, "sysstatus");
    u->lastrate = julong(j, "lastrate");
    u->nuv = julong(j, "nuv");
    u->timebank = julong(j, "timebank");

    /* Floats */
    u->timeontoday = jfloat(j, "timeontoday");
    u->extratime = jfloat(j, "extratime");
    u->timeon = jfloat(j, "timeon");
    u->pcr = jfloat(j, "pcr");
    u->ratio = jfloat(j, "ratio");
    u->pos_account = jfloat(j, "pos_account");
    u->neg_account = jfloat(j, "neg_account");

    /* Reserved */
    reserved = cJSON_GetObjectItemCaseSensitive(j, "_reserved");
    if (reserved) {
        get_char_byte_array(reserved, "res", u->res, 29);
        get_long_array(reserved, "resl", u->resl, 29);
        get_int_array(reserved, "resi", u->resi, 29);
        get_float_array(reserved, "resf", u->resf, 29);
    }

    /* Scan pointers */
    get_long_array(j, "qscn", u->qscn, 200);
    get_long_array(j, "nscn", u->nscn, 200);
}

/* ================================================================
 * slrec / valrec / arcrec helpers (nested in configrec)
 * ================================================================ */

static cJSON *slrec_to_json(slrec *s)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(obj, "time_per_day", s->time_per_day);
    cJSON_AddNumberToObject(obj, "time_per_logon", s->time_per_logon);
    cJSON_AddNumberToObject(obj, "maxcalls", s->maxcalls);
    cJSON_AddNumberToObject(obj, "emails", s->emails);
    cJSON_AddNumberToObject(obj, "posts", s->posts);
    cJSON_AddNumberToObject(obj, "ability", (double)s->ability);
    return obj;
}

static void json_to_slrec(cJSON *j, slrec *s)
{
    s->time_per_day = (unsigned short)jint(j, "time_per_day");
    s->time_per_logon = (unsigned short)jint(j, "time_per_logon");
    s->maxcalls = (unsigned short)jint(j, "maxcalls");
    s->emails = (unsigned short)jint(j, "emails");
    s->posts = (unsigned short)jint(j, "posts");
    s->ability = julong(j, "ability");
}

static cJSON *valrec_to_json(valrec *v)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(obj, "sl", v->sl);
    cJSON_AddNumberToObject(obj, "dsl", v->dsl);
    cJSON_AddNumberToObject(obj, "ar", v->ar);
    cJSON_AddNumberToObject(obj, "dar", v->dar);
    cJSON_AddNumberToObject(obj, "restrict", v->restrict);
    return obj;
}

static void json_to_valrec(cJSON *j, valrec *v)
{
    v->sl = (unsigned char)jint(j, "sl");
    v->dsl = (unsigned char)jint(j, "dsl");
    v->ar = (unsigned short)jint(j, "ar");
    v->dar = (unsigned short)jint(j, "dar");
    v->restrict = (unsigned short)jint(j, "restrict");
}

static cJSON *arcrec_to_json(arcrec *a)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "extension", a->extension);
    cJSON_AddStringToObject(obj, "arca", a->arca);
    cJSON_AddStringToObject(obj, "arce", a->arce);
    cJSON_AddStringToObject(obj, "arcl", a->arcl);
    return obj;
}

static void json_to_arcrec(cJSON *j, arcrec *a)
{
    memset(a, 0, sizeof(arcrec));
    jstr(j, "extension", a->extension, sizeof(a->extension));
    jstr(j, "arca", a->arca, sizeof(a->arca));
    jstr(j, "arce", a->arce, sizeof(a->arce));
    jstr(j, "arcl", a->arcl, sizeof(a->arcl));
}

/* exarc (in niftyrec) */
static cJSON *exarc_to_json(exarc *a)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "arct", a->arct);
    cJSON_AddStringToObject(obj, "arcc", a->arcc);
    return obj;
}

static void json_to_exarc(cJSON *j, exarc *a)
{
    memset(a, 0, sizeof(exarc));
    jstr(j, "arct", a->arct, sizeof(a->arct));
    jstr(j, "arcc", a->arcc, sizeof(a->arcc));
}

/* ================================================================
 * configrec + niftyrec <-> JSON
 * ================================================================ */

cJSON *configrec_to_json(configrec *c, niftyrec *n)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *config, *nifty_obj, *sl_arr, *av_arr, *arcs_arr;
    int i;

    /* --- configrec --- */
    config = cJSON_CreateObject();

    /* String fields */
    cJSON_AddStringToObject(config, "newuserpw", c->newuserpw);
    cJSON_AddStringToObject(config, "systempw", c->systempw);
    cJSON_AddStringToObject(config, "msgsdir", c->msgsdir);
    cJSON_AddStringToObject(config, "gfilesdir", c->gfilesdir);
    cJSON_AddStringToObject(config, "datadir", c->datadir);
    cJSON_AddStringToObject(config, "dloadsdir", c->dloadsdir);
    cJSON_AddStringToObject(config, "tempdir", c->tempdir);
    cJSON_AddStringToObject(config, "menudir", c->menudir);
    cJSON_AddStringToObject(config, "bbs_init_modem", c->bbs_init_modem);
    cJSON_AddStringToObject(config, "answer", c->answer);
    cJSON_AddStringToObject(config, "no_carrier", c->no_carrier);
    cJSON_AddStringToObject(config, "ring", c->ring);
    cJSON_AddStringToObject(config, "terminal", c->terminal);
    cJSON_AddStringToObject(config, "systemname", c->systemname);
    cJSON_AddStringToObject(config, "systemphone", c->systemphone);
    cJSON_AddStringToObject(config, "sysopname", c->sysopname);
    cJSON_AddStringToObject(config, "executestr", c->executestr);
    cJSON_AddStringToObject(config, "hangupphone", c->hangupphone);
    cJSON_AddStringToObject(config, "pickupphone", c->pickupphone);
    cJSON_AddStringToObject(config, "connect_300_a", c->connect_300_a);
    cJSON_AddStringToObject(config, "beginday_c", c->beginday_c);
    cJSON_AddStringToObject(config, "logon_c", c->logon_c);
    cJSON_AddStringToObject(config, "newuser_c", c->newuser_c);
    cJSON_AddStringToObject(config, "dial_prefix", c->dial_prefix);
    cJSON_AddStringToObject(config, "upload_c", c->upload_c);
    cJSON_AddStringToObject(config, "dszbatchdl", c->dszbatchdl);
    cJSON_AddStringToObject(config, "modem_type", c->modem_type);
    cJSON_AddStringToObject(config, "batchdir", c->batchdir);

    /* Char field */
    {
        char rd[2];
        rd[0] = c->ramdrive;
        rd[1] = '\0';
        cJSON_AddStringToObject(config, "ramdrive", rd);
    }
    {
        char nt[2];
        nt[0] = c->network_type;
        nt[1] = '\0';
        cJSON_AddStringToObject(config, "network_type", nt);
    }

    /* Unsigned char scalars */
    cJSON_AddNumberToObject(config, "newusersl", c->newusersl);
    cJSON_AddNumberToObject(config, "newuserdsl", c->newuserdsl);
    cJSON_AddNumberToObject(config, "maxwaiting", c->maxwaiting);
    add_uchar_array(config, "comport", c->comport, 5);
    add_uchar_array(config, "com_ISR", c->com_ISR, 5);
    cJSON_AddNumberToObject(config, "primaryport", c->primaryport);
    cJSON_AddNumberToObject(config, "newuploads", c->newuploads);
    cJSON_AddNumberToObject(config, "closedsystem", c->closedsystem);

    /* Unsigned short scalars */
    cJSON_AddNumberToObject(config, "systemnumber", c->systemnumber);
    add_ushort_array(config, "baudrate", c->baudrate, 5);
    add_ushort_array(config, "com_base", c->com_base, 5);
    cJSON_AddNumberToObject(config, "maxusers", c->maxusers);
    cJSON_AddNumberToObject(config, "newuser_restrict", c->newuser_restrict);
    cJSON_AddNumberToObject(config, "sysconfig", c->sysconfig);
    add_flags(config, "sysconfig_flags", c->sysconfig, sysconfig_flags, N_SYSCONFIG_FLAGS);
    cJSON_AddNumberToObject(config, "sysoplowtime", c->sysoplowtime);
    cJSON_AddNumberToObject(config, "sysophightime", c->sysophightime);
    cJSON_AddNumberToObject(config, "executetime", c->executetime);

    /* Floats */
    cJSON_AddNumberToObject(config, "req_ratio", (double)c->req_ratio);
    cJSON_AddNumberToObject(config, "newusergold", (double)c->newusergold);
    cJSON_AddNumberToObject(config, "post_call_ratio", (double)c->post_call_ratio);

    /* sl[256] — only emit non-zero entries */
    sl_arr = cJSON_CreateObject();
    for (i = 0; i < 256; i++) {
        if (c->sl[i].time_per_day || c->sl[i].time_per_logon ||
            c->sl[i].emails || c->sl[i].posts || c->sl[i].ability) {
            char key[8];
            snprintf(key, sizeof(key), "%d", i);
            cJSON_AddItemToObject(sl_arr, key, slrec_to_json(&c->sl[i]));
        }
    }
    cJSON_AddItemToObject(config, "sl", sl_arr);

    /* autoval[10] */
    av_arr = cJSON_CreateArray();
    for (i = 0; i < 10; i++)
        cJSON_AddItemToArray(av_arr, valrec_to_json(&c->autoval[i]));
    cJSON_AddItemToObject(config, "autoval", av_arr);

    /* arcs[4] */
    arcs_arr = cJSON_CreateArray();
    for (i = 0; i < 4; i++)
        cJSON_AddItemToArray(arcs_arr, arcrec_to_json(&c->arcs[i]));
    cJSON_AddItemToObject(config, "arcs", arcs_arr);

    /* Unsigned ints */
    cJSON_AddNumberToObject(config, "netlowtime", c->netlowtime);
    cJSON_AddNumberToObject(config, "nethightime", c->nethightime);

    /* Internal offsets (preserved for compatibility) */
    cJSON_AddNumberToObject(config, "userreclen", c->userreclen);
    cJSON_AddNumberToObject(config, "waitingoffset", c->waitingoffset);
    cJSON_AddNumberToObject(config, "inactoffset", c->inactoffset);
    cJSON_AddNumberToObject(config, "sysstatusoffset", c->sysstatusoffset);

    /* wwiv_reg_number */
    cJSON_AddNumberToObject(config, "wwiv_reg_number", (double)c->wwiv_reg_number);

    cJSON_AddItemToObject(root, "config", config);

    /* --- niftyrec --- */
    nifty_obj = cJSON_CreateObject();

    cJSON_AddNumberToObject(nifty_obj, "chatcolor", (int)(unsigned char)n->chatcolor);
    cJSON_AddNumberToObject(nifty_obj, "echochar", (int)(unsigned char)n->echochar);
    cJSON_AddStringToObject(nifty_obj, "firstmenu", n->firstmenu);
    cJSON_AddNumberToObject(nifty_obj, "matrixtype", (int)(unsigned char)n->matrixtype);
    cJSON_AddNumberToObject(nifty_obj, "fcom", (int)(unsigned char)n->fcom);
    cJSON_AddStringToObject(nifty_obj, "newusermenu", n->newusermenu);
    cJSON_AddNumberToObject(nifty_obj, "systemtype", (int)(unsigned char)n->systemtype);
    cJSON_AddStringToObject(nifty_obj, "menudir", n->menudir);
    cJSON_AddNumberToObject(nifty_obj, "fptsratio", (int)(unsigned char)n->fptsratio);
    cJSON_AddNumberToObject(nifty_obj, "lockoutrate", (double)n->lockoutrate);
    cJSON_AddNumberToObject(nifty_obj, "nifstatus", (double)n->nifstatus);
    add_flags(nifty_obj, "nifstatus_flags", n->nifstatus, nifstatus_flags, N_NIFSTATUS_FLAGS);
    add_char_byte_array(nifty_obj, "rotate", n->rotate, 5);

    /* arc[4] (exarc) */
    {
        cJSON *ea = cJSON_CreateArray();
        for (i = 0; i < 4; i++)
            cJSON_AddItemToArray(ea, exarc_to_json(&n->arc[i]));
        cJSON_AddItemToObject(nifty_obj, "arc", ea);
    }

    cJSON_AddStringToObject(nifty_obj, "matrix", n->matrix);
    cJSON_AddStringToObject(nifty_obj, "lockoutpw", n->lockoutpw);

    /* NUV fields */
    cJSON_AddNumberToObject(nifty_obj, "nuvbad", (int)(unsigned char)n->nuvbad);
    cJSON_AddNumberToObject(nifty_obj, "nuvyes", (int)(unsigned char)n->nuvyes);
    cJSON_AddNumberToObject(nifty_obj, "nuvlevel", (int)(unsigned char)n->nuvlevel);
    add_char_byte_array(nifty_obj, "nuvsl", n->nuvsl, 10);
    add_char_byte_array(nifty_obj, "nuvinf", n->nuvinf, 8);
    add_char_byte_array(nifty_obj, "nuinf", n->nuinf, 8);
    cJSON_AddNumberToObject(nifty_obj, "nulevel", (int)(unsigned char)n->nulevel);
    add_char_byte_array(nifty_obj, "defaultcol", n->defaultcol, 20);
    cJSON_AddNumberToObject(nifty_obj, "nuvaction", (int)(unsigned char)n->nuvaction);
    cJSON_AddNumberToObject(nifty_obj, "nuvbadlevel", (int)(unsigned char)n->nuvbadlevel);

    cJSON_AddItemToObject(root, "nifty", nifty_obj);

    return root;
}

void json_to_configrec(cJSON *j, configrec *c, niftyrec *n)
{
    cJSON *config, *nifty_obj, *sl_obj, *item, *av_arr, *arcs_arr;
    int i;

    memset(c, 0, sizeof(configrec));
    memset(n, 0, sizeof(niftyrec));

    config = cJSON_GetObjectItemCaseSensitive(j, "config");
    if (!config) return;

    /* Strings */
    jstr(config, "newuserpw", c->newuserpw, sizeof(c->newuserpw));
    jstr(config, "systempw", c->systempw, sizeof(c->systempw));
    jstr(config, "msgsdir", c->msgsdir, sizeof(c->msgsdir));
    jstr(config, "gfilesdir", c->gfilesdir, sizeof(c->gfilesdir));
    jstr(config, "datadir", c->datadir, sizeof(c->datadir));
    jstr(config, "dloadsdir", c->dloadsdir, sizeof(c->dloadsdir));
    jstr(config, "tempdir", c->tempdir, sizeof(c->tempdir));
    jstr(config, "menudir", c->menudir, sizeof(c->menudir));
    jstr(config, "bbs_init_modem", c->bbs_init_modem, sizeof(c->bbs_init_modem));
    jstr(config, "answer", c->answer, sizeof(c->answer));
    jstr(config, "no_carrier", c->no_carrier, sizeof(c->no_carrier));
    jstr(config, "ring", c->ring, sizeof(c->ring));
    jstr(config, "terminal", c->terminal, sizeof(c->terminal));
    jstr(config, "systemname", c->systemname, sizeof(c->systemname));
    jstr(config, "systemphone", c->systemphone, sizeof(c->systemphone));
    jstr(config, "sysopname", c->sysopname, sizeof(c->sysopname));
    jstr(config, "executestr", c->executestr, sizeof(c->executestr));
    jstr(config, "hangupphone", c->hangupphone, sizeof(c->hangupphone));
    jstr(config, "pickupphone", c->pickupphone, sizeof(c->pickupphone));
    jstr(config, "connect_300_a", c->connect_300_a, sizeof(c->connect_300_a));
    jstr(config, "beginday_c", c->beginday_c, sizeof(c->beginday_c));
    jstr(config, "logon_c", c->logon_c, sizeof(c->logon_c));
    jstr(config, "newuser_c", c->newuser_c, sizeof(c->newuser_c));
    jstr(config, "dial_prefix", c->dial_prefix, sizeof(c->dial_prefix));
    jstr(config, "upload_c", c->upload_c, sizeof(c->upload_c));
    jstr(config, "dszbatchdl", c->dszbatchdl, sizeof(c->dszbatchdl));
    jstr(config, "modem_type", c->modem_type, sizeof(c->modem_type));
    jstr(config, "batchdir", c->batchdir, sizeof(c->batchdir));

    c->ramdrive = jchar(config, "ramdrive");
    c->network_type = jchar(config, "network_type");

    /* Unsigned char scalars */
    c->newusersl = (unsigned char)jint(config, "newusersl");
    c->newuserdsl = (unsigned char)jint(config, "newuserdsl");
    c->maxwaiting = (unsigned char)jint(config, "maxwaiting");
    get_uchar_array(config, "comport", c->comport, 5);
    get_uchar_array(config, "com_ISR", c->com_ISR, 5);
    c->primaryport = (unsigned char)jint(config, "primaryport");
    c->newuploads = (unsigned char)jint(config, "newuploads");
    c->closedsystem = (unsigned char)jint(config, "closedsystem");

    /* Unsigned short scalars */
    c->systemnumber = (unsigned short)jint(config, "systemnumber");
    get_ushort_array(config, "baudrate", c->baudrate, 5);
    get_ushort_array(config, "com_base", c->com_base, 5);
    c->maxusers = (unsigned short)jint(config, "maxusers");
    c->newuser_restrict = (unsigned short)jint(config, "newuser_restrict");
    c->sysconfig = (unsigned short)jint(config, "sysconfig");
    c->sysoplowtime = (unsigned short)jint(config, "sysoplowtime");
    c->sysophightime = (unsigned short)jint(config, "sysophightime");
    c->executetime = (unsigned short)jint(config, "executetime");

    /* Floats */
    c->req_ratio = jfloat(config, "req_ratio");
    c->newusergold = jfloat(config, "newusergold");
    c->post_call_ratio = jfloat(config, "post_call_ratio");

    /* sl[256] — sparse object keyed by level number */
    sl_obj = cJSON_GetObjectItemCaseSensitive(config, "sl");
    if (sl_obj) {
        cJSON *child = sl_obj->child;
        while (child) {
            int level = atoi(child->string);
            if (level >= 0 && level < 256)
                json_to_slrec(child, &c->sl[level]);
            child = child->next;
        }
    }

    /* autoval[10] */
    av_arr = cJSON_GetObjectItemCaseSensitive(config, "autoval");
    if (cJSON_IsArray(av_arr)) {
        i = 0;
        cJSON_ArrayForEach(item, av_arr) {
            if (i >= 10) break;
            json_to_valrec(item, &c->autoval[i]);
            i++;
        }
    }

    /* arcs[4] */
    arcs_arr = cJSON_GetObjectItemCaseSensitive(config, "arcs");
    if (cJSON_IsArray(arcs_arr)) {
        i = 0;
        cJSON_ArrayForEach(item, arcs_arr) {
            if (i >= 4) break;
            json_to_arcrec(item, &c->arcs[i]);
            i++;
        }
    }

    /* Unsigned ints */
    c->netlowtime = (unsigned int)jint(config, "netlowtime");
    c->nethightime = (unsigned int)jint(config, "nethightime");

    /* Internal offsets */
    c->userreclen = jint(config, "userreclen");
    c->waitingoffset = jint(config, "waitingoffset");
    c->inactoffset = jint(config, "inactoffset");
    c->sysstatusoffset = jint(config, "sysstatusoffset");

    c->wwiv_reg_number = julong(config, "wwiv_reg_number");

    /* --- niftyrec --- */
    nifty_obj = cJSON_GetObjectItemCaseSensitive(j, "nifty");
    if (!nifty_obj) return;

    n->chatcolor = (char)jint(nifty_obj, "chatcolor");
    n->echochar = (char)jint(nifty_obj, "echochar");
    jstr(nifty_obj, "firstmenu", n->firstmenu, sizeof(n->firstmenu));
    n->matrixtype = (char)jint(nifty_obj, "matrixtype");
    n->fcom = (char)jint(nifty_obj, "fcom");
    jstr(nifty_obj, "newusermenu", n->newusermenu, sizeof(n->newusermenu));
    n->systemtype = (char)jint(nifty_obj, "systemtype");
    jstr(nifty_obj, "menudir", n->menudir, sizeof(n->menudir));
    n->fptsratio = (char)jint(nifty_obj, "fptsratio");
    n->lockoutrate = julong(nifty_obj, "lockoutrate");
    n->nifstatus = julong(nifty_obj, "nifstatus");
    get_char_byte_array(nifty_obj, "rotate", n->rotate, 5);

    /* arc[4] (exarc) */
    {
        cJSON *ea = cJSON_GetObjectItemCaseSensitive(nifty_obj, "arc");
        if (cJSON_IsArray(ea)) {
            i = 0;
            cJSON_ArrayForEach(item, ea) {
                if (i >= 4) break;
                json_to_exarc(item, &n->arc[i]);
                i++;
            }
        }
    }

    jstr(nifty_obj, "matrix", n->matrix, sizeof(n->matrix));
    jstr(nifty_obj, "lockoutpw", n->lockoutpw, sizeof(n->lockoutpw));

    n->nuvbad = (char)jint(nifty_obj, "nuvbad");
    n->nuvyes = (char)jint(nifty_obj, "nuvyes");
    n->nuvlevel = (char)jint(nifty_obj, "nuvlevel");
    get_char_byte_array(nifty_obj, "nuvsl", n->nuvsl, 10);
    get_char_byte_array(nifty_obj, "nuvinf", n->nuvinf, 8);
    get_char_byte_array(nifty_obj, "nuinf", n->nuinf, 8);
    n->nulevel = (char)jint(nifty_obj, "nulevel");
    get_char_byte_array(nifty_obj, "defaultcol", n->defaultcol, 20);
    n->nuvaction = (char)jint(nifty_obj, "nuvaction");
    n->nuvbadlevel = (char)jint(nifty_obj, "nuvbadlevel");
}

/* ================================================================
 * statusrec <-> JSON
 * ================================================================ */

cJSON *statusrec_to_json(statusrec *s)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "date1", s->date1);
    cJSON_AddStringToObject(root, "date2", s->date2);
    cJSON_AddStringToObject(root, "date3", s->date3);
    cJSON_AddStringToObject(root, "log1", s->log1);
    cJSON_AddStringToObject(root, "log2", s->log2);
    cJSON_AddNumberToObject(root, "dltoday", (int)(unsigned char)s->dltoday);
    cJSON_AddStringToObject(root, "log5", s->log5);
    cJSON_AddNumberToObject(root, "users", s->users);
    cJSON_AddNumberToObject(root, "callernum", s->callernum);
    cJSON_AddNumberToObject(root, "callstoday", s->callstoday);
    cJSON_AddNumberToObject(root, "msgposttoday", s->msgposttoday);
    cJSON_AddNumberToObject(root, "emailtoday", s->emailtoday);
    cJSON_AddNumberToObject(root, "fbacktoday", s->fbacktoday);
    cJSON_AddNumberToObject(root, "uptoday", s->uptoday);
    cJSON_AddNumberToObject(root, "activetoday", s->activetoday);
    cJSON_AddNumberToObject(root, "qscanptr", (double)s->qscanptr);
    {
        char aa[2];
        aa[0] = s->amsganon;
        aa[1] = '\0';
        cJSON_AddStringToObject(root, "amsganon", aa);
    }
    cJSON_AddNumberToObject(root, "amsguser", s->amsguser);
    cJSON_AddNumberToObject(root, "callernum1", (double)s->callernum1);
    cJSON_AddNumberToObject(root, "net_edit_stuff", s->net_edit_stuff);
    cJSON_AddNumberToObject(root, "wwiv_version", s->wwiv_version);
    cJSON_AddNumberToObject(root, "net_version", s->net_version);
    cJSON_AddNumberToObject(root, "net_bias", (double)s->net_bias);
    cJSON_AddNumberToObject(root, "last_connect", (double)s->last_connect);
    cJSON_AddNumberToObject(root, "last_bbslist", (double)s->last_bbslist);
    cJSON_AddNumberToObject(root, "net_req_free", (double)s->net_req_free);
    cJSON_AddStringToObject(root, "log3", s->log3);
    cJSON_AddStringToObject(root, "log4", s->log4);
    cJSON_AddStringToObject(root, "lastuser", s->lastuser);

    return root;
}

void json_to_statusrec(cJSON *j, statusrec *s)
{
    memset(s, 0, sizeof(statusrec));

    jstr(j, "date1", s->date1, sizeof(s->date1));
    jstr(j, "date2", s->date2, sizeof(s->date2));
    jstr(j, "date3", s->date3, sizeof(s->date3));
    jstr(j, "log1", s->log1, sizeof(s->log1));
    jstr(j, "log2", s->log2, sizeof(s->log2));
    s->dltoday = (char)jint(j, "dltoday");
    jstr(j, "log5", s->log5, sizeof(s->log5));
    s->users = (unsigned short)jint(j, "users");
    s->callernum = (unsigned short)jint(j, "callernum");
    s->callstoday = (unsigned short)jint(j, "callstoday");
    s->msgposttoday = (unsigned short)jint(j, "msgposttoday");
    s->emailtoday = (unsigned short)jint(j, "emailtoday");
    s->fbacktoday = (unsigned short)jint(j, "fbacktoday");
    s->uptoday = (unsigned short)jint(j, "uptoday");
    s->activetoday = (unsigned short)jint(j, "activetoday");
    s->qscanptr = julong(j, "qscanptr");
    s->amsganon = jchar(j, "amsganon");
    s->amsguser = (unsigned short)jint(j, "amsguser");
    s->callernum1 = (unsigned long)julong(j, "callernum1");
    s->net_edit_stuff = (unsigned int)jint(j, "net_edit_stuff");
    s->wwiv_version = (unsigned int)jint(j, "wwiv_version");
    s->net_version = (unsigned int)jint(j, "net_version");
    s->net_bias = jfloat(j, "net_bias");
    s->last_connect = (long)julong(j, "last_connect");
    s->last_bbslist = (long)julong(j, "last_bbslist");
    s->net_req_free = jfloat(j, "net_req_free");
    jstr(j, "log3", s->log3, sizeof(s->log3));
    jstr(j, "log4", s->log4, sizeof(s->log4));
    jstr(j, "lastuser", s->lastuser, sizeof(s->lastuser));
}
