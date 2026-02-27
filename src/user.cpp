/*
 * user.cpp — User class implementation
 *
 * display_name() replaces the old nam()/pnam() free functions.
 * to_json()/from_json() own the JSON serialization format.
 */

#include "user.h"
#include "platform.h"
#include "cJSON.h"

#include <cstdio>
#include <cstring>


std::string User::display_name() const
{
    char o[MAX_PATH_LEN];
    int f = 1;
    int p;

    for (p = 0; p < (int)std::strlen(data_.name); p++) {
        if (f) {
            if ((data_.name[p] >= 'A') && (data_.name[p] <= 'Z'))
                f = 0;
            o[p] = data_.name[p];
        } else {
            if ((data_.name[p] >= 'A') && (data_.name[p] <= 'Z'))
                o[p] = data_.name[p] - 'A' + 'a';
            else {
                if ((data_.name[p] >= ' ') && (data_.name[p] <= '/'))
                    f = 1;
                o[p] = data_.name[p];
            }
        }
    }
    o[p] = 0;
    return std::string(o);
}


std::string User::display_name(int usernum) const
{
    char o[MAX_PATH_LEN];
    int f = 1;
    int p;

    for (p = 0; p < (int)std::strlen(data_.name); p++) {
        if (f) {
            if ((data_.name[p] >= 'A') && (data_.name[p] <= 'Z'))
                f = 0;
            o[p] = data_.name[p];
        } else {
            if ((data_.name[p] >= 'A') && (data_.name[p] <= 'Z'))
                o[p] = data_.name[p] - 'A' + 'a';
            else {
                if ((data_.name[p] >= ' ') && (data_.name[p] <= '/'))
                    f = 1;
                o[p] = data_.name[p];
            }
        }
    }
    o[p++] = ' ';
    o[p++] = '#';
    itoa(usernum, &o[p], 10);
    return std::string(o);
}


/* ================================================================
 * JSON serialization helpers (file-static)
 * ================================================================ */

static void jstr(cJSON *obj, const char *key, char *dest, int maxlen)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsString(item) && item->valuestring) {
        strncpy(dest, item->valuestring, maxlen - 1);
        dest[maxlen - 1] = '\0';
    }
}

static int jint(cJSON *obj, const char *key)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsNumber(item))
        return item->valueint;
    return 0;
}

static unsigned long julong(cJSON *obj, const char *key)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsNumber(item))
        return (unsigned long)item->valuedouble;
    return 0;
}

static float jfloat(cJSON *obj, const char *key)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsNumber(item))
        return (float)item->valuedouble;
    return 0.0f;
}

static char jchar(cJSON *obj, const char *key)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsString(item) && item->valuestring && item->valuestring[0])
        return item->valuestring[0];
    if (cJSON_IsNumber(item))
        return (char)item->valueint;
    return 0;
}

static void add_uchar_array(cJSON *obj, const char *key,
                            const unsigned char *arr, int count)
{
    cJSON *a = cJSON_CreateArray();
    for (int i = 0; i < count; i++)
        cJSON_AddItemToArray(a, cJSON_CreateNumber(arr[i]));
    cJSON_AddItemToObject(obj, key, a);
}

static void get_uchar_array(cJSON *obj, const char *key,
                            unsigned char *arr, int count)
{
    cJSON *a = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (!cJSON_IsArray(a)) return;
    int i = 0;
    cJSON *item;
    cJSON_ArrayForEach(item, a) {
        if (i >= count) break;
        if (cJSON_IsNumber(item)) arr[i] = (unsigned char)item->valueint;
        i++;
    }
}

static void add_long_array(cJSON *obj, const char *key,
                           const long *arr, int count)
{
    cJSON *a = cJSON_CreateArray();
    for (int i = 0; i < count; i++)
        cJSON_AddItemToArray(a, cJSON_CreateNumber((double)arr[i]));
    cJSON_AddItemToObject(obj, key, a);
}

static void get_long_array(cJSON *obj, const char *key,
                           long *arr, int count)
{
    cJSON *a = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (!cJSON_IsArray(a)) return;
    int i = 0;
    cJSON *item;
    cJSON_ArrayForEach(item, a) {
        if (i >= count) break;
        if (cJSON_IsNumber(item)) arr[i] = (long)item->valuedouble;
        i++;
    }
}

static void add_int_array(cJSON *obj, const char *key,
                          const int *arr, int count)
{
    cJSON *a = cJSON_CreateArray();
    for (int i = 0; i < count; i++)
        cJSON_AddItemToArray(a, cJSON_CreateNumber(arr[i]));
    cJSON_AddItemToObject(obj, key, a);
}

static void get_int_array(cJSON *obj, const char *key,
                          int *arr, int count)
{
    cJSON *a = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (!cJSON_IsArray(a)) return;
    int i = 0;
    cJSON *item;
    cJSON_ArrayForEach(item, a) {
        if (i >= count) break;
        if (cJSON_IsNumber(item)) arr[i] = item->valueint;
        i++;
    }
}

static void add_float_array(cJSON *obj, const char *key,
                            const float *arr, int count)
{
    cJSON *a = cJSON_CreateArray();
    for (int i = 0; i < count; i++)
        cJSON_AddItemToArray(a, cJSON_CreateNumber((double)arr[i]));
    cJSON_AddItemToObject(obj, key, a);
}

static void get_float_array(cJSON *obj, const char *key,
                            float *arr, int count)
{
    cJSON *a = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (!cJSON_IsArray(a)) return;
    int i = 0;
    cJSON *item;
    cJSON_ArrayForEach(item, a) {
        if (i >= count) break;
        if (cJSON_IsNumber(item)) arr[i] = (float)item->valuedouble;
        i++;
    }
}

static void add_char_byte_array(cJSON *obj, const char *key,
                                const char *arr, int count)
{
    cJSON *a = cJSON_CreateArray();
    for (int i = 0; i < count; i++)
        cJSON_AddItemToArray(a, cJSON_CreateNumber((int)(unsigned char)arr[i]));
    cJSON_AddItemToObject(obj, key, a);
}

static void get_char_byte_array(cJSON *obj, const char *key,
                                char *arr, int count)
{
    cJSON *a = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (!cJSON_IsArray(a)) return;
    int i = 0;
    cJSON *item;
    cJSON_ArrayForEach(item, a) {
        if (i >= count) break;
        if (cJSON_IsNumber(item)) arr[i] = (char)item->valueint;
        i++;
    }
}

/* Flag tables for human-readable JSON */
typedef struct { unsigned long bit; const char *name; } flagdef;

static void add_flags(cJSON *obj, const char *key,
                      unsigned long val, flagdef *defs, int ndefs)
{
    cJSON *a = cJSON_CreateArray();
    for (int i = 0; i < ndefs; i++) {
        if (val & defs[i].bit)
            cJSON_AddItemToArray(a, cJSON_CreateString(defs[i].name));
    }
    cJSON_AddItemToObject(obj, key, a);
}

static flagdef sysstatus_flagdefs[] = {
    { 0x0001, "ansi" },     { 0x0002, "color" },
    { 0x0004, "fullline" }, { 0x0008, "pause_on_page" },
    { 0x0020, "smw" },
    { 0x0040, "full_screen" }, { 0x0080, "nscan_file_system" },
    { 0x0100, "regular" },  { 0x0200, "clr_scrn" }
};

static flagdef restrict_flagdefs[] = {
    { 0x0001, "logon" },    { 0x0002, "chat" },
    { 0x0004, "validate" }, { 0x0008, "automessage" },
    { 0x0010, "anony" },    { 0x0020, "post" },
    { 0x0040, "email" },    { 0x0080, "vote" },
    { 0x0100, "auto_msg_delete" }, { 0x0200, "net" },
    { 0x0400, "upload" },   { 0x0800, "rumours" },
    { 0x1000, "timebank" }, { 0x2000, "bbslist" },
    { 0x4000, "userlist" }
};

static flagdef inact_flagdefs[] = {
    { 0x01, "deleted" }, { 0x02, "inactive" }, { 0x04, "lockedout" }
};

static flagdef exempt_flagdefs[] = {
    { 0x01, "ratio" }, { 0x02, "time" },
    { 0x04, "userlist" }, { 0x08, "post" }
};

static const char *ar_letters = "ABCDEFGHIJKLMNOP";

static void add_ar_flags(cJSON *obj, const char *key, unsigned short val)
{
    cJSON *a = cJSON_CreateArray();
    for (int i = 0; i < 16; i++) {
        if (val & (1 << i)) {
            char s[2] = { ar_letters[i], '\0' };
            cJSON_AddItemToArray(a, cJSON_CreateString(s));
        }
    }
    cJSON_AddItemToObject(obj, key, a);
}


/* ================================================================
 * User::to_json — serialize to cJSON tree using typed accessors
 * ================================================================ */

cJSON* User::to_json() const
{
    cJSON *root = cJSON_CreateObject();

    /* String fields */
    cJSON_AddStringToObject(root, "name", name());
    cJSON_AddStringToObject(root, "realname", realname());
    cJSON_AddStringToObject(root, "callsign", callsign());
    cJSON_AddStringToObject(root, "phone", phone());
    cJSON_AddStringToObject(root, "dphone", dataphone());
    cJSON_AddStringToObject(root, "pw", password());
    cJSON_AddStringToObject(root, "laston", laston());
    cJSON_AddStringToObject(root, "firston", firston());
    cJSON_AddStringToObject(root, "note", note());
    cJSON_AddStringToObject(root, "comment", comment());
    cJSON_AddStringToObject(root, "street", street());
    cJSON_AddStringToObject(root, "city", city());
    {
        char sex_str[2] = { sex(), '\0' };
        cJSON_AddStringToObject(root, "sex", sex_str);
    }

    /* Macros — 4 string fields */
    {
        cJSON *macros_arr = cJSON_CreateArray();
        for (int i = 0; i < 4; i++)
            cJSON_AddItemToArray(macros_arr, cJSON_CreateString(macro(i)));
        cJSON_AddItemToObject(root, "macros", macros_arr);
    }

    /* Unsigned char scalars */
    cJSON_AddNumberToObject(root, "age", age());
    cJSON_AddNumberToObject(root, "inact", inact());
    add_flags(root, "inact_flags", inact(), inact_flagdefs,
              sizeof(inact_flagdefs) / sizeof(inact_flagdefs[0]));
    cJSON_AddNumberToObject(root, "comp_type", comp_type());
    cJSON_AddNumberToObject(root, "defprot", defprot());
    cJSON_AddNumberToObject(root, "defed", defed());
    cJSON_AddNumberToObject(root, "flisttype", flisttype());
    cJSON_AddNumberToObject(root, "mlisttype", mlisttype());
    cJSON_AddNumberToObject(root, "helplevel", helplevel());
    cJSON_AddNumberToObject(root, "lastsub", lastsub());
    cJSON_AddNumberToObject(root, "lastdir", lastdir());
    cJSON_AddNumberToObject(root, "lastconf", lastconf());
    cJSON_AddNumberToObject(root, "screenchars", screenchars());
    cJSON_AddNumberToObject(root, "screenlines", screenlines());
    cJSON_AddNumberToObject(root, "sl", sl());
    cJSON_AddNumberToObject(root, "dsl", dsl());
    cJSON_AddNumberToObject(root, "exempt", exempt());
    add_flags(root, "exempt_flags", exempt(), exempt_flagdefs,
              sizeof(exempt_flagdefs) / sizeof(exempt_flagdefs[0]));
    add_uchar_array(root, "colors", colors(), 20);
    add_uchar_array(root, "votes", votes(), 20);
    cJSON_AddNumberToObject(root, "illegal", illegal());
    cJSON_AddNumberToObject(root, "waiting", waiting());
    cJSON_AddNumberToObject(root, "subop", subop());
    cJSON_AddNumberToObject(root, "ontoday", ontoday());

    /* Unsigned short scalars */
    cJSON_AddNumberToObject(root, "forwardusr", forwardusr());
    cJSON_AddNumberToObject(root, "msgpost", msgpost());
    cJSON_AddNumberToObject(root, "emailsent", emailsent());
    cJSON_AddNumberToObject(root, "feedbacksent", feedbacksent());
    cJSON_AddNumberToObject(root, "posttoday", posttoday());
    cJSON_AddNumberToObject(root, "etoday", etoday());
    cJSON_AddNumberToObject(root, "ar", ar());
    add_ar_flags(root, "ar_flags", ar());
    cJSON_AddNumberToObject(root, "dar", dar());
    add_ar_flags(root, "dar_flags", dar());
    cJSON_AddNumberToObject(root, "restrict", restrict_flags());
    add_flags(root, "restrict_flags", restrict_flags(), restrict_flagdefs,
              sizeof(restrict_flagdefs) / sizeof(restrict_flagdefs[0]));
    cJSON_AddNumberToObject(root, "month", birth_month());
    cJSON_AddNumberToObject(root, "day", birth_day());
    cJSON_AddNumberToObject(root, "year", birth_year());

    /* int */
    cJSON_AddNumberToObject(root, "fpts", fpts());

    /* More unsigned shorts */
    cJSON_AddNumberToObject(root, "uploaded", uploaded());
    cJSON_AddNumberToObject(root, "downloaded", downloaded());
    cJSON_AddNumberToObject(root, "logons", logons());
    cJSON_AddNumberToObject(root, "fsenttoday1", fsenttoday1());
    cJSON_AddNumberToObject(root, "emailnet", emailnet());
    cJSON_AddNumberToObject(root, "postnet", postnet());

    /* Unsigned longs */
    cJSON_AddNumberToObject(root, "msgread", (double)msgread());
    cJSON_AddNumberToObject(root, "uk", (double)uk());
    cJSON_AddNumberToObject(root, "dk", (double)dk());
    cJSON_AddNumberToObject(root, "daten", (double)daten());
    cJSON_AddNumberToObject(root, "sysstatus", (double)sysstatus());
    add_flags(root, "sysstatus_flags", sysstatus(), sysstatus_flagdefs,
              sizeof(sysstatus_flagdefs) / sizeof(sysstatus_flagdefs[0]));
    cJSON_AddNumberToObject(root, "lastrate", (double)lastrate());
    cJSON_AddNumberToObject(root, "nuv", (double)nuv_status());
    cJSON_AddNumberToObject(root, "timebank", (double)timebank());

    /* Floats */
    cJSON_AddNumberToObject(root, "timeontoday", (double)timeontoday());
    cJSON_AddNumberToObject(root, "extratime", (double)extratime());
    cJSON_AddNumberToObject(root, "timeon", (double)total_timeon());
    cJSON_AddNumberToObject(root, "pcr", (double)pcr());
    cJSON_AddNumberToObject(root, "ratio", (double)ul_dl_ratio());
    cJSON_AddNumberToObject(root, "pos_account", (double)pos_account());
    cJSON_AddNumberToObject(root, "neg_account", (double)neg_account());

    /* Reserved arrays — only include if non-zero */
    {
        int has_res = 0;
        for (int i = 0; i < 29; i++)
            if (res()[i] || resl()[i] || resi()[i] || resf()[i] != 0.0f)
                has_res = 1;
        if (has_res) {
            cJSON *reserved = cJSON_CreateObject();
            add_char_byte_array(reserved, "res", res(), 29);
            add_long_array(reserved, "resl", resl(), 29);
            add_int_array(reserved, "resi", resi(), 29);
            add_float_array(reserved, "resf", resf(), 29);
            cJSON_AddItemToObject(root, "_reserved", reserved);
        }
    }

    /* qscn and nscn — message scan pointers */
    add_long_array(root, "qscn", qscn(), 200);
    add_long_array(root, "nscn", nscn(), 200);

    return root;
}


/* ================================================================
 * User::from_json — deserialize from cJSON tree using typed setters
 * ================================================================ */

User User::from_json(cJSON *j)
{
    User u;
    cJSON *item;

    /* Strings — use direct buffer access since set_*() copies in */
    jstr(j, "name", u.data_.name, sizeof(u.data_.name));
    jstr(j, "realname", u.data_.realname, sizeof(u.data_.realname));
    jstr(j, "callsign", u.data_.callsign, sizeof(u.data_.callsign));
    jstr(j, "phone", u.data_.phone, sizeof(u.data_.phone));
    jstr(j, "dphone", u.data_.dphone, sizeof(u.data_.dphone));
    jstr(j, "pw", u.data_.pw, sizeof(u.data_.pw));
    jstr(j, "laston", u.data_.laston, sizeof(u.data_.laston));
    jstr(j, "firston", u.data_.firston, sizeof(u.data_.firston));
    jstr(j, "note", u.data_.note, sizeof(u.data_.note));
    jstr(j, "comment", u.data_.comment, sizeof(u.data_.comment));
    jstr(j, "street", u.data_.street, sizeof(u.data_.street));
    jstr(j, "city", u.data_.city, sizeof(u.data_.city));
    u.set_sex(jchar(j, "sex"));

    /* Macros */
    item = cJSON_GetObjectItemCaseSensitive(j, "macros");
    if (cJSON_IsArray(item)) {
        int i = 0;
        cJSON *m;
        cJSON_ArrayForEach(m, item) {
            if (i >= 4) break;
            if (cJSON_IsString(m) && m->valuestring) {
                strncpy(u.data_.macros[i], m->valuestring, MAX_PATH_LEN - 1);
                u.data_.macros[i][MAX_PATH_LEN - 1] = '\0';
            }
            i++;
        }
    }

    /* Unsigned char scalars */
    u.set_age((unsigned char)jint(j, "age"));
    u.set_inact((unsigned char)jint(j, "inact"));
    u.set_comp_type((unsigned char)jint(j, "comp_type"));
    u.set_defprot((unsigned char)jint(j, "defprot"));
    u.set_defed((unsigned char)jint(j, "defed"));
    u.set_flisttype((unsigned char)jint(j, "flisttype"));
    u.set_mlisttype((unsigned char)jint(j, "mlisttype"));
    u.set_helplevel((unsigned char)jint(j, "helplevel"));
    u.set_lastsub((unsigned char)jint(j, "lastsub"));
    u.set_lastdir((unsigned char)jint(j, "lastdir"));
    u.set_lastconf((unsigned char)jint(j, "lastconf"));
    u.set_screenchars((unsigned char)jint(j, "screenchars"));
    u.set_screenlines((unsigned char)jint(j, "screenlines"));
    u.set_sl((unsigned char)jint(j, "sl"));
    u.set_dsl((unsigned char)jint(j, "dsl"));
    u.set_exempt((unsigned char)jint(j, "exempt"));
    get_uchar_array(j, "colors", u.colors_mut(), 20);
    get_uchar_array(j, "votes", u.votes_mut(), 20);
    u.set_illegal((unsigned char)jint(j, "illegal"));
    u.set_waiting((unsigned char)jint(j, "waiting"));
    u.set_subop((unsigned char)jint(j, "subop"));
    u.set_ontoday((unsigned char)jint(j, "ontoday"));

    /* Unsigned short scalars */
    u.set_forwardusr((unsigned short)jint(j, "forwardusr"));
    u.set_msgpost((unsigned short)jint(j, "msgpost"));
    u.set_emailsent((unsigned short)jint(j, "emailsent"));
    u.set_feedbacksent((unsigned short)jint(j, "feedbacksent"));
    u.set_posttoday((unsigned short)jint(j, "posttoday"));
    u.set_etoday((unsigned short)jint(j, "etoday"));
    u.set_ar((unsigned short)jint(j, "ar"));
    u.set_dar((unsigned short)jint(j, "dar"));
    u.set_restrict((unsigned short)jint(j, "restrict"));
    u.set_birth_month((unsigned short)jint(j, "month"));
    u.set_birth_day((unsigned short)jint(j, "day"));
    u.set_birth_year((unsigned short)jint(j, "year"));

    /* int */
    u.set_fpts(jint(j, "fpts"));

    /* More unsigned shorts */
    u.set_uploaded((unsigned short)jint(j, "uploaded"));
    u.set_downloaded((unsigned short)jint(j, "downloaded"));
    u.set_logons((unsigned short)jint(j, "logons"));
    u.set_fsenttoday1((unsigned short)jint(j, "fsenttoday1"));
    u.set_emailnet((unsigned short)jint(j, "emailnet"));
    u.set_postnet((unsigned short)jint(j, "postnet"));

    /* Unsigned longs */
    u.set_msgread(julong(j, "msgread"));
    u.set_uk(julong(j, "uk"));
    u.set_dk(julong(j, "dk"));
    u.set_daten(julong(j, "daten"));
    u.set_sysstatus(julong(j, "sysstatus"));
    u.set_lastrate(julong(j, "lastrate"));
    u.set_nuv_status(julong(j, "nuv"));
    u.set_timebank(julong(j, "timebank"));

    /* Floats */
    u.set_timeontoday(jfloat(j, "timeontoday"));
    u.set_extratime(jfloat(j, "extratime"));
    u.set_total_timeon(jfloat(j, "timeon"));
    u.set_pcr(jfloat(j, "pcr"));
    u.set_ul_dl_ratio(jfloat(j, "ratio"));
    u.set_pos_account(jfloat(j, "pos_account"));
    u.set_neg_account(jfloat(j, "neg_account"));

    /* Reserved */
    cJSON *reserved = cJSON_GetObjectItemCaseSensitive(j, "_reserved");
    if (reserved) {
        get_char_byte_array(reserved, "res", u.res_mut(), 29);
        get_long_array(reserved, "resl", u.resl_mut(), 29);
        get_int_array(reserved, "resi", u.resi_mut(), 29);
        get_float_array(reserved, "resf", u.resf_mut(), 29);
    }

    /* Scan pointers */
    get_long_array(j, "qscn", u.qscn_mut(), 200);
    get_long_array(j, "nscn", u.nscn_mut(), 200);

    /* Null-terminate safety (replaces fix_user_rec) */
    u.data_.name[30] = 0;
    u.data_.realname[20] = 0;
    u.data_.callsign[6] = 0;
    u.data_.phone[12] = 0;
    u.data_.pw[19] = 0;
    u.data_.note[40] = 0;
    u.data_.macros[0][80] = 0;
    u.data_.macros[1][80] = 0;
    u.data_.macros[2][80] = 0;

    return u;
}
